#ifndef _DBEXCEPTION_HPP_
#define _DBEXCEPTION_HPP_
#include <iostream>
#include <cstring>
using namespace std;

const size_t MAX_MSG_LEN = 200;
const int FILE_NOT_MATCH = 100;
const int FILE_NOT_EXIST = 101;
class dbException{
protected:
    char msg[MAX_MSG_LEN];
    int errCode = 0;
public:
    dbException(){}
    virtual ~dbException(){}
    virtual char *what() const{}
    virtual int code() const{}
};

class fileNotMatch : public dbException{
public:
    fileNotMatch(){
        strcpy(msg, "Error : FILE NOT MATCH!");
        errCode = FILE_NOT_MATCH;
    }

    char *what(){
        return msg;
    }

    int code(){
        return errCode;
    }
};

class ImportFileNotExist : public dbException{
public:
    ImportFileNotExist(){
        strcpy(msg, "Error : Import file not exist! If you want to create new file, please specify the dataLen!");
        errCode = FILE_NOT_EXIST;
    }

    char *what(){
        return msg;
    }

    int code(){
        return errCode;
    }
};
#endif
