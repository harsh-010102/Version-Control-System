#include <iostream>
#include <filesystem>
#include <fstream>
#include <string>
#include <vector>
#include <zlib.h>
#include <sys/stat.h>

#include <openssl/sha.h>
#include <iomanip>
#include "utils.h"


using namespace std;
constexpr size_t BUFFER_SIZE = 16384;

using namespace std;

string readFile(string filePath) {
    ifstream inputFile(filePath, ios::binary);
    if (!inputFile) {
        throw runtime_error("Failed to open file: " + filePath);
    }

    ostringstream contentStream;
    contentStream << inputFile.rdbuf();
    inputFile.close();
    return contentStream.str();
}



void decompressZlib(string &fileSha, string option) {
    // Open input file
    string folder = fileSha.substr(0, 2);
    string file = fileSha.substr(2);

    string inputFile = "./.git/objects/" + folder + '/' + file;

    ifstream input(inputFile, ios::binary);
    if (!input) {
        throw runtime_error(" File didn't exists: " + inputFile);
        return;
    }

    // Initialize zlib inflate stream
    z_stream strm{};
    strm.zalloc = Z_NULL;
    strm.zfree = Z_NULL;
    strm.opaque = Z_NULL;

    if (inflateInit(&strm) != Z_OK) {
        throw runtime_error("Failed to initialize zlib.");
        return;
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
            inflateEnd(&strm);
            throw runtime_error("Error reading input file.");
            return;
        }

        strm.next_in = reinterpret_cast<Bytef*>(inBuffer.data());

        // Decompress until all input is processed
        do {
            strm.avail_out = BUFFER_SIZE;
            strm.next_out = reinterpret_cast<Bytef*>(outBuffer.data());

            result = inflate(&strm, Z_NO_FLUSH);

            if (result == Z_MEM_ERROR || result == Z_DATA_ERROR || result == Z_STREAM_ERROR) {
                inflateEnd(&strm);
                throw runtime_error("Decompression error: "  + result);
                return ;
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
                        decompressZlibTree(fileSha, false);
                        return; 
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
                inflateEnd(&strm);
                throw runtime_error("Error writing to standard output.");
                return;
            }

        } while (strm.avail_out == 0);
    }

    inflateEnd(&strm);

    if (result != Z_STREAM_END) {
        throw runtime_error("Decompression did not complete successfully.");
        return;
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

    return;
}

string getShaOfContent(string &content) {
    string hash(SHA_DIGEST_LENGTH, '\0');  // Create a string with SHA_DIGEST_LENGTH (20) null characters
    SHA1(reinterpret_cast<const unsigned char*>(content.c_str()), content.size(), reinterpret_cast<unsigned char*>(&hash[0]));
    return hash;  
}


string getHexSha(string &sha) {
    ostringstream sha_hexadecimal;
    for (unsigned char char_ : sha) {
        sha_hexadecimal << std::hex << std::setw(2) << std::setfill('0') << (int)(char_);
    }
    return sha_hexadecimal.str();
}

string compressContent(string &content) {
    vector<char> compressedBuffer(compressBound(content.size()));

    uLongf compressedSize = compressedBuffer.size();
    int result = compress(reinterpret_cast<Bytef*>(compressedBuffer.data()), &compressedSize,
                          reinterpret_cast<const Bytef*>(content.data()), content.size());

    if (result != Z_OK) {
        throw std::runtime_error("Compression failed!");
    }

    return string(compressedBuffer.data(), compressedSize);
}

void storeCompressDataInFile(string &content, string &sha1) {
    string directory = ".git/objects/" + sha1.substr(0, 2);
    string filename = sha1.substr(2);
    // cout << directory << "\n";
    mkdir(directory.c_str(), 0777);

    string filepath = directory + "/" + filename;
    ofstream outfile(filepath, ios::binary);

    outfile.write(content.c_str(), content.size());
    outfile.close();
}

void decompressZlibTree(string &sha, bool option) {
    string dirName = sha.substr(0, 2); 
    string fileName = sha.substr(2);  
    string inputFilePath =  "./.git/objects/" + dirName + "/" + fileName;

    string compressedData = readFile(inputFilePath);
    z_stream stream;
    stream.zalloc = Z_NULL;
    stream.zfree = Z_NULL;
    stream.opaque = Z_NULL;
    stream.avail_in = compressedData.size();
    stream.next_in = reinterpret_cast<Bytef *>(const_cast<char *>(compressedData.data()));

    if (inflateInit(&stream) != Z_OK) {
        throw runtime_error("Failed to initialize zlib");
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
            throw runtime_error("Error during decompression");
            return;
        }
        
        uncompressedData.append(buffer, sizeof(buffer) - stream.avail_out);
    } while (ret != Z_STREAM_END);

    inflateEnd(&stream);
    if(uncompressedData.substr(0, 4) == "blob") {
        throw runtime_error("fatal: not a tree object");
    }
    else if(uncompressedData.substr(0, 6) == "commit") {
        
        int idx = uncompressedData.find('\0');
        uncompressedData = uncompressedData.substr(idx + 1);
        
        idx = uncompressedData.find(' '); 
        string newSHA = uncompressedData.substr(idx + 1, 40);

        decompressZlibTree(newSHA, option);

        return;
    }
    parseTreeObject(uncompressedData, option);
    return;
}

void parseTreeObject(string &uncompressedData, bool option) {
    size_t pos = 0;

    // Skip the "tree <size>\0" part
    while (pos < uncompressedData.size() && uncompressedData[pos] != '\0') {
        pos++;
    }
    pos++;

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


static bool comp(const filesystem::directory_entry &p1, const filesystem::directory_entry &p2) {
    return p1.path().filename().string() < p2.path().filename().string();
}


string writeTreeRec(filesystem::path path) {
    if (filesystem::is_empty(path)) {
        return string("");
    }
    vector<filesystem::directory_entry> entries;

    for (auto it : filesystem::directory_iterator(path)){
        if(it.path().filename() == ".git" || it.path().filename() == "build" || it.path().filename() == "CMakeFiles" ) {
            continue;
        }
        entries.push_back(it);
    }

    sort(entries.begin(), entries.end(), comp);

    ostringstream tree_body;
    
    for(auto it : entries) {
        
        if(it.is_directory()) {
            string mode = "40000";  
            string name = it.path().filename().string();

            string directory_sha = writeTreeRec(it.path());

            tree_body << mode << " " << name << '\0' << directory_sha;
        }
        else {
            struct stat fileStat;
    
            auto perm = filesystem::status(it).permissions();
            unsigned int perm_value = static_cast<unsigned int>(perm) & 0777;  
            // cout << to_string(perm_value) << "\n";
            string mode = "100644";
            // cout << mode << "\n";
            
            string content = readFile(it.path().string());
            string blobHeader = "blob " + to_string(content.size()) + '\0';

            string blobContent = blobHeader + content;
            string sha = getShaOfContent(blobContent);

            tree_body << mode << " " << it.path().filename().string()<< '\0' << sha;

            string hexSha = getHexSha(sha);
            string compressData = compressContent(blobContent);
            storeCompressDataInFile(compressData, hexSha); 
               
        }
    }
    string tree = "tree " + to_string(tree_body.str().size()) + '\0' + tree_body.str();
    string shaBytes = getShaOfContent(tree);
    string sha = getHexSha(shaBytes);
    // cout << tree  << "\n"; 

    string compressedContent = compressContent(tree);

    storeCompressDataInFile(compressedContent, sha);
    return shaBytes;
}

void writeTree(filesystem::path &path) {
    string sha = writeTreeRec(path);
    string hexSHA = getHexSha(sha); 
    cout << hexSHA << "\n";
    return;
} 

string exec(const char* cmd) {
    char buffer[128];
    std::string result = "";
    FILE* pipe = popen(cmd, "r");
    if (!pipe) return "Error";
    while (fgets(buffer, sizeof(buffer), pipe) != nullptr) {
        result += buffer;
    }
    pclose(pipe);
    return result;
}

pair<string, string> getUserInfo() {
    const char* homeDir = getenv("HOME");  
    string path = string(homeDir) + "/.gitconfig";

    string gitConfigContent = readFile(path);

    string userName, userEmail;
    istringstream stream(gitConfigContent);
    string line;

    while (getline(stream, line)) {
        if (line.find("name = ") != string::npos) {
            userName = line.substr(line.find("name = ") + 7); // Extract the name value
        }
        if (line.find("email = ") != string::npos) {
            userEmail = line.substr(line.find("email = ") + 8); // Extract the email value
        }
    }

    return {userName, userEmail};
}

string getTimeStamp() {
    string unixTimestamp = exec("git log -1 --format=%at");
    string fullDate = exec("git log -1 --format=%ai");

    unixTimestamp.erase(remove(unixTimestamp.begin(), unixTimestamp.end(), '\n'), unixTimestamp.end());
    fullDate.erase(remove(fullDate.begin(), fullDate.end(), '\n'), fullDate.end());

    size_t spacePos = fullDate.rfind(' ');
    string timezoneOffset;
    if (spacePos != string::npos) {
        timezoneOffset = fullDate.substr(spacePos + 1);
    }
    string unixTime = unixTimestamp + " " + timezoneOffset;
    return unixTime;
}
void commitTree(string &treeSha, string &parentSha, string &msg) {
    pair<string, string> p = getUserInfo();

    string userName = p.first;
    string email = p.second;

    string unixTimestamp = getTimeStamp();
    // cout << unixTimestamp << "\n";

    ostringstream commitContent;

    commitContent << "tree " << treeSha << "\n";
    if(parentSha.size() > 0) {
        commitContent <<  "parent  " << parentSha << "\n";
    }
    commitContent << "author " << userName << " " << email << " " << unixTimestamp << "\n";
    commitContent << "committer  " << userName << " " << email << " " << unixTimestamp << "\n";

    commitContent << "\n" << msg << "\n";

    // cout << commitContent.str();

    string commitHeader = "commit " + to_string(commitContent.str().size()) + '\0';
    string commitBody = commitHeader + commitContent.str();

    string commitSha = getShaOfContent(commitBody);
    string commitHexSha = getHexSha(commitSha);

    cout << commitHexSha << "\n";

    string compressCommitData = compressContent(commitBody);
    storeCompressDataInFile(compressCommitData, commitHexSha);
    return;
}

