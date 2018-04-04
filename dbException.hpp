#ifndef _DBEXCEPTION_HPP_
#define _DBEXCEPTION_HPP_
#include <iostream>
#include <cstring>
using namespace std;

const size_t MAX_MSG_LEN = 50;
const int FILE_NOT_MATCH = 100;
class dbException{
protected:
    char msg[MAX_MSG_LEN];
    int errCode = 0;
public:
    dbException(){}
    virtual ~dbException(){}
    virtual char *what() const = 0;
    virtual int code() const = 0;
};

class fileNotMatch : public dbException{
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
#endif
