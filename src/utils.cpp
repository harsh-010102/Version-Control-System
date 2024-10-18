#include <iostream>
#include <filesystem>
#include <fstream>
#include <string>
#include <vector>
#include <zlib.h>
#include <openssl/sha.h>
#include <iomanip>
#include "utils.h"


using namespace std;
constexpr size_t BUFFER_SIZE = 16384;

using namespace std;

string readFile(string& filePath) {
    ifstream inputFile(filePath, ios::binary);
    if (!inputFile) {
        cerr << "Failed to open file: " << filePath;
    }

    ostringstream contentStream;
    contentStream << inputFile.rdbuf();

    return contentStream.str();
}


void decompressZlibTree(string &inputFile, bool option) {
    string compressedData = readFile(inputFile);
    z_stream stream;
    stream.zalloc = Z_NULL;
    stream.zfree = Z_NULL;
    stream.opaque = Z_NULL;
    stream.avail_in = compressedData.size();
    stream.next_in = reinterpret_cast<Bytef *>(const_cast<char *>(compressedData.data()));

    if (inflateInit(&stream) != Z_OK) {
        cerr << "Failed to initialize zlib";
        return;
    }

    string uncompressedData;
    char buffer[BUFFER_SIZE];
    int ret;

    do {
        stream.avail_out = sizeof(buffer);
        stream.next_out = reinterpret_cast<Bytef *>(buffer);
        ret = inflate(&stream, Z_NO_FLUSH);

        if (ret == Z_MEM_ERROR || ret == Z_DATA_ERROR || ret == Z_STREAM_ERROR) {
            inflateEnd(&stream);
            cerr << "Error during decompression";
            return;
        }
        
        uncompressedData.append(buffer, sizeof(buffer) - stream.avail_out);
    } while (ret != Z_STREAM_END);

    inflateEnd(&stream);
    if(uncompressedData.substr(0, 4) != "tree") {
        cout << "fatal: not a tree object" << "\n";
        return;

    }
    parseTreeObject(uncompressedData, option);
    return;
}

void parseTreeObject(string &uncompressedData, bool option) {
    size_t pos = 0;

    // Skip the "tree <size>\0" part
    while (pos < uncompressedData.size() && uncompressedData[pos] != '\0') {
        ++pos;
    }
    ++pos;

    while (pos < uncompressedData.size()) {

        string mode;
        while (uncompressedData[pos] != ' ') {
            mode.push_back(uncompressedData[pos]);
            pos++;
        }
        pos++;

        // Read the filename
        string fileName;
        while (uncompressedData[pos] != '\0') {
            fileName.push_back(uncompressedData[pos]);
            pos++;
        }
        pos++;

        if(option) {
            cout << fileName << "\n";
            pos += 20;
            continue;
        }
        string sha1_hex;
        for (int i = 0; i < 20; ++i) {
            unsigned char byte = static_cast<unsigned char>(uncompressedData[pos + i]);
            ostringstream oss;
            oss << hex << setw(2) << setfill('0') << static_cast<int>(byte);
            sha1_hex += oss.str();
        }
        pos += 20;

        // Print the parsed entry
        string type;
        if(mode.substr(0, 3) == "100" || mode.substr(0, 3) == "120") {
            type = "blob";
        }
        else if(mode.substr(0, 3) == "160") {
            type = "commit";
        }
        else {
            mode.insert(mode.begin(), '0');
            type = "tree";
        }
        
        cout << mode  << " " << type << " " << sha1_hex << "    " << fileName << "\n";
    }
}


bool decompressZlib(string &inputFile, string option) {
    // Open input file
    ifstream input(inputFile, ios::binary);
    if (!input) {
        cerr << "File didn't exists: " << inputFile << endl;
        return false;
    }

    // Initialize zlib inflate stream
    z_stream strm{};
    strm.zalloc = Z_NULL;
    strm.zfree = Z_NULL;
    strm.opaque = Z_NULL;

    if (inflateInit(&strm) != Z_OK) {
        cerr << "Failed to initialize zlib." << endl;
        return false;
    }

    vector<char> inBuffer(BUFFER_SIZE);
    vector<char> outBuffer(BUFFER_SIZE);

    int result = Z_OK;
    string header;
    size_t decompressedSize = 0;
    string fileHeader;

    while (result != Z_STREAM_END) {
        input.read(inBuffer.data(), BUFFER_SIZE);
        strm.avail_in = static_cast<uInt>(input.gcount());

        if (input.fail() && !input.eof()) {
            cerr << "Error reading input file." << endl;
            inflateEnd(&strm);
            return false;
        }

        strm.next_in = reinterpret_cast<Bytef*>(inBuffer.data());

        // Decompress until all input is processed
        do {
            strm.avail_out = BUFFER_SIZE;
            strm.next_out = reinterpret_cast<Bytef*>(outBuffer.data());

            result = inflate(&strm, Z_NO_FLUSH);

            if (result == Z_MEM_ERROR || result == Z_DATA_ERROR || result == Z_STREAM_ERROR) {
                cerr << "Decompression error: " << result << endl;
                inflateEnd(&strm);
                return false;
            }

            size_t have = BUFFER_SIZE - strm.avail_out;

            if (header.empty()) {
                // Extract the header to skip it
                header.append(outBuffer.data(), have);
            
                size_t pos = header.find('\0');

                if (pos != string::npos) {
                    // Extract file type from header
                    fileHeader = header.substr(0, pos);

                    if(fileHeader.substr(0, 4) == "tree") {
                        decompressZlibTree(inputFile, false);
                        return true; 
                    } 

                    string remainingData = header.substr(pos + 1);
                    decompressedSize += remainingData.size();
                    
                    if (option == "-p") {
                        cout.write(remainingData.data(), remainingData.size());
                    }

                    header.clear();
                }
            } 
            else {
                decompressedSize += have;

                if (option == "-p") {
                    cout.write(outBuffer.data(), have);
                    
                }
            }

            if (cout.fail()) {
                cerr << "Error writing to standard output." << endl;
                inflateEnd(&strm);
                return false;
            }

        } while (strm.avail_out == 0);
    }

    inflateEnd(&strm);

    if (result != Z_STREAM_END) {
        cerr << "Decompression did not complete successfully." << endl;
        return false;
    }   

    // Handle other options
    int idx = fileHeader.find(' ');
    string fileType = fileHeader.substr(0, idx);

    
    if (option == "-s") {
        cout << decompressedSize << "\n";
    } 
    else if (option == "-t") {
        cout << fileType << "\n";
    }

    return true;
}


bool compressFile(string& inputFilePath, string& outputFilePath) {
    // Open the input file
    ifstream inputFile(inputFilePath, ios::binary);
    if (!inputFile) {
        cerr << "Failed to open input file: " << inputFilePath << "\n";
        return false;
    }

    // Get the input file size
    filesystem::path filePath(inputFilePath);
    uint64_t fileSize = filesystem::file_size(filePath);

    // Read the input file content
    string content((istreambuf_iterator<char>(inputFile)), istreambuf_iterator<char>());
    
    // Prepare to write the compressed data
    ofstream outputFile(outputFilePath, ios::binary);
    if (!outputFile) {
        cerr << "Failed to create output file: " << outputFilePath << "\n";
        return false;
    }

    // Create the header
    ostringstream headerStream;
    headerStream << "blob " << fileSize << '\0';
    string header = headerStream.str();

    // Combine header and content
    string finalBuffer = header + content;

    // Initialize zlib stream
    z_stream strm;
    strm.zalloc = Z_NULL;
    strm.zfree = Z_NULL;
    strm.opaque = Z_NULL;

    // Initialize the compression level
    if (deflateInit(&strm, Z_BEST_SPEED) != Z_OK) {
        cerr << "Failed to initialize zlib for compression\n";
        return false;
    }

    // Set the input data
    strm.avail_in = finalBuffer.size();
    strm.next_in = reinterpret_cast<Bytef*>(const_cast<char*>(finalBuffer.data()));

    // Prepare a buffer for the compressed data
    vector<char> compressedBuffer(deflateBound(&strm, finalBuffer.size()));
    strm.avail_out = compressedBuffer.size();
    strm.next_out = reinterpret_cast<Bytef*>(compressedBuffer.data());

    // Compress the data
    int ret = deflate(&strm, Z_FINISH);
    if (ret != Z_STREAM_END) {
        deflateEnd(&strm);
        cerr << "Compression failed\n";
        return false;
    }

    // Calculate the size of the compressed data
    size_t compressedSize = compressedBuffer.size() - strm.avail_out;

    // Write the compressed data to the output file
    outputFile.write(compressedBuffer.data(), compressedSize);

    // Clean up
    deflateEnd(&strm);
    return true;
}


string sha1(string& filename) {
    ifstream file(filename, ios::binary);
    if (!file) {
        cerr << "Could not open file: " << filename << endl;
        return "";
    } 

    // Determine the file size
    file.seekg(0, ios::end);
    size_t fileSize = file.tellg();
    file.seekg(0, ios::beg);

    // Create the header: "blob <filesize>\0"
    string header = "blob " + to_string(fileSize) + '\0';

    SHA_CTX sha1;
    SHA1_Init(&sha1);

    // Update the hash with the header
    SHA1_Update(&sha1, header.c_str(), header.size());

    // Read the file and update the hash
    const size_t bufferSize = 8192;
    char buffer[bufferSize];

    while (file.read(buffer, bufferSize)) {
        SHA1_Update(&sha1, buffer, file.gcount());
    }
    // Process the remaining bytes if any
    SHA1_Update(&sha1, buffer, file.gcount());

    unsigned char hash[SHA_DIGEST_LENGTH];
    SHA1_Final(hash, &sha1);

    // Convert the hash to a hexadecimal string
    ostringstream hexStream;
    for (int i = 0; i < SHA_DIGEST_LENGTH; ++i) {
        hexStream << hex << setw(2) << setfill('0') << static_cast<int>(hash[i]);
    }

    return hexStream.str();
}
