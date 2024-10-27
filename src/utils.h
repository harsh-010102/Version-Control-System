#ifndef UTILS_H
#define UTILS_H

#include <string>
#include <bits/stdc++.h>
#include <openssl/sha.h>
#include <iomanip>

using namespace std;


struct IndexEntry {
    string mode;
    string sha;
    filesystem::path filePath;
};


// utils functions

string readFile(string filePath);


void decompressZlib(string &fileSha, string option);
string compressContent(string &content);

string getShaOfContent(string &content);
string getHexSha(string &sha);

void storeCompressDataInFile(string &content, string &sha1);


void decompressZlibTree(string &sha, bool option);
void parseTreeObject(string &uncompressedData, bool option);

string writeTreeRec(filesystem::path path);

pair<string, string> getUserInfo();
string exec(const char* cmd);
string getTimeStamp();

void commitTree(string &treeSha, string &parentSha, string &msg);

//git add functions 
void updateIndex(map<string, string> stageFileInfo);
pair<string, string>  stageFile(const std::string &filePath);
void gitAddDirectory(const std::filesystem::path &dirPath, map<string, string> &stageFileInfo);
void addAllFiles();
void addAllFiles(vector<filesystem::path> vec);

// commit functions :

map<filesystem::path, pair<string, string>> readIndexFile(string indexFilePath);
map<filesystem::path, vector<IndexEntry>> groupEntriesByDirectory(vector<IndexEntry> entries);
void writeTree(std::filesystem::path &path);

void writeTree(map<filesystem::path, pair<string, string>>  &stagedFilesInfo, filesystem::path &path, string msg);

void checkOut(string& commitSHA);
#endif // UTILS_H

