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
        if(argc < 3) {
            cout << "Incorrect command(git cat-file options <file-sha>)";
            return EXIT_FAILURE;
        }

        string option = argv[2];
        
        if(option != "-p" && option != "-t" && option != "-s") {
            cout << "Incorrect command(git cat-file options <file-sha>)";
            return EXIT_FAILURE;
        }
   
        string fileSha = argv[3];
        string folder = fileSha.substr(0, 2);
        string file = fileSha.substr(2); 

        string filePath = "./.git/objects/" + folder + '/' + file;
        
        vector<string> vec;

        decompressZlib(filePath, option);
    
    }

    else if(command == "hash-object") {
        if(argc < 4) {
            cout << "Incorrect command(git hash-object options <file-name>)";
            return EXIT_FAILURE;
        }

        string option = argv[2];
        string fileName = argv[3];

        string fileSHA = sha1(fileName);
    
        string objectFolderName = "./.git/objects/" + fileSHA.substr(0, 2);
        string objectFileName = objectFolderName + "/" + fileSHA.substr(2);
        cout << objectFileName << "\n";

        if(option == "-w") {
            // cout << "Here " << "\n";
            if (!filesystem::exists(objectFolderName)) {
                // cout << "Folder created\n";
                filesystem::create_directories(objectFolderName);
            }

            ofstream outFile(objectFileName, ios::binary);
            
            if (!outFile) {
                cerr << "Failed to create file: " << objectFileName << "\n";
                return EXIT_FAILURE;
            }   

            if (!compressFile(fileName, objectFileName)) {
                return EXIT_FAILURE;
            }             

            outFile.close();
        }

        cout << fileSHA << "\n";
    }

    else if(command == "ls-tree") {
        if(argc != 3 && argc != 4) {
            cout << "Incorrect command(git ls-tree --name-only(optional) <file-sha>)";
            return EXIT_FAILURE;
        }
        string treeSHA;

        bool option = false;
        if(string(argv[2]) == "--name-only") {
            // cout << argc;
            if(argc != 4) {
            cout << "Incorrect command(git ls-tree --name-only <file-sha>)";
            return EXIT_FAILURE;
            }
            treeSHA = argv[3];
            option = true;
        }
        else {
            treeSHA = argv[2];
            option = false;
        }
        string objectFolderName = "./.git/objects/" + treeSHA.substr(0, 2);
        string objectFileName = objectFolderName + "/" + treeSHA.substr(2);

        decompressZlibTree(objectFileName, option);
    }
    else {
        cerr << "Unknown command " << command << '\n';
        return EXIT_FAILURE;
    }
    
    return EXIT_SUCCESS;
}
// /home/harsh/Desktop/Version_control/codecrafters-git-cpp/.git/objects/9d