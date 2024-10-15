#ifndef UTILS_H
#define UTILS_H

#include <string>
#include <bits/stdc++.h>
#include <openssl/sha.h>
#include <iomanip>

using namespace std;

string readFile(string filePath);


void decompressZlib(string &fileSha, string option);
string compressContent(string &content);

string getShaOfContent(string &content);
string getHexSha(string &sha);

void storeCompressDataInFile(string &content, string &sha1);


void decompressZlibTree(string &sha, bool option);
void parseTreeObject(string &uncompressedData, bool option);

string writeTreeRec(filesystem::path path);
void writeTree(filesystem::path &path);

pair<string, string> getUserInfo();
string exec(const char* cmd);
string getTimeStamp();

void commitTree(string &treeSha, string &parentSha, string &msg);


#endif // UTILS_H
