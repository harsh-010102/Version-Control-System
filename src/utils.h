#ifndef UTILS_H
#define UTILS_H

#include <string>
#include <bits/stdc++.h>
#include <openssl/sha.h>
#include <iomanip>

using namespace std;
bool decompressZlib(string &inputFile, string option);
bool compressFile(string& inputFilePath, string& outputFilePath);
string sha1(string& filename);
void decompressZlibTree(string &inputFile, bool option);
void parseTreeObject(string &uncompressedData, bool option);


#endif // UTILS_H
