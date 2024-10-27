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
            filesystem::create_directory(".git/refs/heads");
            filesystem::create_directory(".git/refs/heads");
            filesystem::create_directory(".git/logs");

            filesystem::create_directory(".git/logs/refs");
            filesystem::create_directory(".git/logs/refs/heads");

    
            ofstream headFile(".git/HEAD");

            if (headFile.is_open()) {
                headFile << "ref: refs/heads/main\n";
                headFile.close();
            } else {
                cerr << "Failed to create .git/HEAD file.\n";
                return EXIT_FAILURE;
            }
            headFile.close();
    
            cout << "Initialized git directory\n";
        } 
        catch (const filesystem::filesystem_error& e) {
            cerr << e.what() << '\n';
            return EXIT_FAILURE;
        }
    }

    else {
        if(!filesystem::exists("./.git") || !filesystem::is_directory("./.git")) {
            cerr << "Incorrect command (parentSha should be of 40 char)" << endl;
            return EXIT_FAILURE;
        }
        if(command == "cat-file") {
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
            
                // cout << objectFileName << "\n";
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

        else if(command == "commit-tree") {
            // cout << argc;
            try {
                if (argc != 5 && argc != 7) {
                    cerr << "Incorrect command git commit-tree requires 5 or 7 arguments" << endl;
                    return EXIT_FAILURE;
                }
                string treeSha;
                string parentSha;
                string msg; 
                if(argc == 5) {
                    if(string(argv[3]) != "-m") {
                        cerr << "Incorrect command (git commit-tree <treeSha> -m message)" << endl;
                        return EXIT_FAILURE;
                    }
                    if(string(argv[2]).size() != 40) {
                        cerr << "Incorrect command (treeSha should be of 40 char)" << endl;
                        return EXIT_FAILURE;                    
                    }
                    treeSha = argv[2];
                    msg = argv[4];
                }
                else if(argc == 7) {
                    if(string(argv[3]) != "-p") {
                        cerr << "Incorrect command (git commit-tree <treeSha> -p <parentSha> -m message)" << endl;
                        return EXIT_FAILURE;
                    }
    
                    if(string(argv[5]) != "-m") {
                        cerr << "Incorrect command (git commit-tree <treeSha> -p <parentSha> -m message)" << endl;
                        return EXIT_FAILURE;
                    }
                    if(string(argv[2]).size() != 40) {
                        cerr << "Incorrect command (treeSha should be of 40 char)" << endl;
                        return EXIT_FAILURE;                    
                    }

                    if(string(argv[4]).size() != 40) {
                        cerr << "Incorrect command (parentSha should be of 40 char)" << endl;
                        return EXIT_FAILURE;                    
                    }

                    treeSha = argv[2];
                    parentSha = argv[4];
                    msg = argv[6];
                }
                commitTree(treeSha, parentSha, msg);
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


        else if(command == "add") {
            try {
                if (argc < 3) {
                    cerr << "Incorrect command git add-tree requires min 3 arguments" << endl;
                    return EXIT_FAILURE;
                }
                string fileName = argv[2];
                vector<filesystem::path> vec;
                if(fileName == ".") {
                    addAllFiles();
                }
                else {
                    for(int i = 2; i < argc; i++) {
                        string path = argv[i];
                        if(filesystem::exists(path)) {
                            vec.push_back(filesystem::relative(path));
                        }
                        else {
                            throw runtime_error("Fatal " + path + " doesn't exist");
                            return EXIT_FAILURE;
                        }
                    }
                    addAllFiles(vec);
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

        else if(command == "commit") {
            try {
                if (argc < 4) {
                    cerr << "Incorrect command git commit requires 4 arguments(git commit -m <commit-msg>)" << endl;
                    return EXIT_FAILURE;
                }
                string msg = argv[3];

                map<filesystem::path, pair<string, string>>  stagedFilesInfo = readIndexFile(".git/index");

                // for(auto it : stagedFilesInfo) {
                //     cout << it.first << "\n";
                // }
                filesystem::path cwd = filesystem :: current_path();
                
                writeTree(stagedFilesInfo, cwd, msg);            
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

        else if(command == "checkout") {
            try {
                if (argc < 3) {
                    cerr << "Incorrect command git checkout requires 3 arguments(git checkout <commit sha>)" << endl;
                    return EXIT_FAILURE;
                }
                string commitSha = argv[2];
                checkOut(commitSha);
                

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
    }

    
    
    return EXIT_SUCCESS;
}
// /home/harsh/Desktop/Version_control/codecrafters-git-cpp/.git/objects/9d;