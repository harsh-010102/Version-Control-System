#include <iostream>
#include <filesystem>
#include <fstream>
#include <string>
#include <bits/stdc++.h>
#include <zlib.h>
#include "utils.h"


using namespace std;

int main(int argc, char *argv[])
{
    // Flush after every cout / cerr
    cout << unitbuf;
    cerr << unitbuf;

    // You can use print statements as follows for debugging, they'll be visible when running tests.
    // cout << "Logs from your program will appear here!\n";

    if (argc < 2) {
        cerr << "No command provided.\n";
        return EXIT_FAILURE;
    }
    
    string command = argv[1];

    // cout << command << "  ";
    if (command == "init") {
        try {
            filesystem::create_directory(".git");
            filesystem::create_directory(".git/objects");
            filesystem::create_directory(".git/refs");
    
            ofstream headFile(".git/HEAD");

            if (headFile.is_open()) {
                headFile << "ref: refs/heads/main\n";
                headFile.close();
            } else {
                cerr << "Failed to create .git/HEAD file.\n";
                return EXIT_FAILURE;
            }
    
            cout << "Initialized git directory\n";
        } 
        catch (const filesystem::filesystem_error& e) {
            cerr << e.what() << '\n';
            return EXIT_FAILURE;
        }
    }

    else if(command == "cat-file") {
        // cout << command;

        try {
            if (argc < 4) {  // Corrected from 3 to 4 to account for fileSHA
                cerr << "Incorrect command(git cat-file options <file-sha>)" << endl;
                return EXIT_FAILURE;
            }

            string option = argv[2];
            
            if (option != "-p" && option != "-t" && option != "-s") {
                cerr << "Incorrect command(git cat-file options <file-sha>)" << endl;
                return EXIT_FAILURE;
            }

            string fileSha = argv[3];
            if(fileSha.size() != 40) {
                cerr << "Invalid Git blob hash length.\n";
                return EXIT_FAILURE;
            }


            decompressZlib(fileSha, option);
        } 
        catch (const runtime_error& e) {
            cerr << "Error: " << e.what() << endl;
            return EXIT_FAILURE;
        }
        catch (const exception& e) {
            cerr << "Unexpected error: " << e.what() << endl;
            return EXIT_FAILURE;
        } 
        catch (...) {
            cerr << "An unknown error occurred." << endl;
            return EXIT_FAILURE;
        }    
    }

    else if (command == "hash-object") {
        try {
            if (argc < 4 || string(argv[2]) != "-w") {
                cout << "Incorrect command(git hash-object -w <file-name>)" << endl;
                return EXIT_FAILURE;
            }

            string option = argv[2];
            string fileName = argv[3];
        
            // cout     << objectFileName << "\n";
            // cout << fileName << "\n";
            string fileContent = readFile(fileName);
            // cout << fileContent << "\n"
            
            string blobHeader = "blob " + to_string(fileContent.size()) + '\0';
            string blobContent = blobHeader + fileContent;

            string sha1 = getShaOfContent(blobContent);
            string hexSha = getHexSha(sha1);
            string compressBlobContent = compressContent(blobContent);
            storeCompressDataInFile(compressBlobContent, hexSha);

            cout << hexSha << "\n";
        } 
        catch (const runtime_error& e) {
            cerr << "Error: " << e.what() << endl;
            return EXIT_FAILURE;
        } 
        catch (const exception& e) {
            cerr << "Unexpected error: " << e.what() << endl;
            return EXIT_FAILURE;
        } 
        catch (...) {
            cerr << "An unknown error occurred." << endl;
            return EXIT_FAILURE;
        }
    }

    else if (command == "ls-tree") {
        try {
           if (argc < 3) {
                cerr << "Incorrect Parameter. git ls-tree --name-only <hash>\n";
                return EXIT_FAILURE;
            } 
            if (argc == 4 && string(argv[2]) != "--name-only") {
                cerr << "Missing parameter: --name-only <hash>\n";
                return EXIT_FAILURE;
            }

            if(argc == 3){
                string hash = argv[2];
                if (hash.size() != 40) {
                    cerr << "Invalid Git blob hash length.\n";
                    return 1;
                }
                decompressZlibTree(hash, false);
            } 
            else {
                string hash = argv[3];
                if (hash.size() != 40) {
                    cerr << "Invalid Git blob hash length.\n";
                    return 1;
                }
                decompressZlibTree(hash, true);
            }
        } 
        catch (const runtime_error& e) {
            cerr << "Error: " << e.what() << endl;
            return EXIT_FAILURE;
        } 
        catch (const exception& e) {
            cerr << "Unexpected error: " << e.what() << endl;
            return EXIT_FAILURE;
        }
        catch (...) {
            cerr << "An unknown error occurred." << endl;
            return EXIT_FAILURE;
        }
    }

    else if(command == "write-tree") {
        try {
            if (argc != 2) {
                cerr << "Incorrect command(git write-tree)" << endl;
                return EXIT_FAILURE;
            }

            filesystem::path current_path = filesystem::current_path();

            writeTree(current_path);
        }
        catch (const runtime_error& e) {
            cerr << "Error: " << e.what() << endl;
            return EXIT_FAILURE;
        } 
        catch (const exception& e) {
            cerr << "Unexpected error: " << e.what() << endl;
            return EXIT_FAILURE;
        }
        catch (...) {
            cerr << "An unknown error occurred." << endl;
            return EXIT_FAILURE;
        }

    }

    else {
        cerr << "Unknown command " << command << '\n';
        return EXIT_FAILURE;
    }
    
    return EXIT_SUCCESS;
}
// /home/harsh/Desktop/Version_control/codecrafters-git-cpp/.git/objects/9d