//
// ODBCbridge Project
// Copyright (C) 2020 by Contributors <https://github.com/Tyill/ODBCbridge>
//
// This code is licensed under the MIT License.
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files(the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and / or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions :
//
// The above copyright notice and this permission notice shall be included in
// all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
// THE SOFTWARE.
//

#pragma once

#if defined(WIN32)
#include <windows.h>
#define SQL_NOUNICODEMAP
#endif

#include <sql.h>
#include <sqlext.h>

#include <cstdint>
#include <algorithm>
#include <string>
#include <vector>
#include <map>

namespace odbc {
   
    struct dbState{
        SQLHENV env = nullptr;
        SQLHDBC hdb = nullptr;
    };   

    std::string _lastError;

#define SQLCALL(f, errMess, ext) {                                         \
      SQLRETURN ret = f;                                              \
      if ((ret != SQL_SUCCESS) && (ret != SQL_SUCCESS_WITH_INFO)){    \
        _lastError = errMess + std::string(" ret ") + to_string(ret); \
        return ext;                                                 \
      }                                                               \
    }

    std::string getLastError(){

        return _lastError;
    }
    
    dbState connect(const std::string& dbServer, const std::string& user, const std::string& password){
        
        dbState st;

        // allocate environment handle
        SQLCALL(SQLAllocHandle(SQL_HANDLE_ENV, SQL_NULL_HANDLE, &st.env), "connect: error environment SQLAllocHandle", dbState());
                
        // register version 
        SQLCALL(SQLSetEnvAttr(st.hdb, SQL_ATTR_ODBC_VERSION, (void*)SQL_OV_ODBC3, 0), "connect: error SQLSetEnvAttr", dbState());
               
        // allocate connection handle
        SQLCALL(SQLAllocHandle(SQL_HANDLE_DBC, st.env, &st.hdb), "connect: error connection SQLAllocHandle", dbState());
                
        // set connect timeout
        SQLCALL(SQLSetConnectAttr(st.hdb, SQL_LOGIN_TIMEOUT, (SQLPOINTER *)5, 0), "connect: error SQLSetConnectAttr", dbState());

        // connect 
        SQLCALL(SQLConnect(st.hdb, (SQLCHAR*)dbServer.c_str(), SQL_NTS,
                                   (SQLCHAR*)user.c_str(), SQL_NTS,
                                   (SQLCHAR*)password.c_str(), SQL_NTS), "connect: error SQLConnect", dbState());
        return st;
    }

    void disconnect(dbState& ioSt){

        if (ioSt.hdb){
            SQLDisconnect(ioSt.hdb);
            SQLFreeHandle(SQL_HANDLE_DBC, ioSt.hdb);
            ioSt.hdb = nullptr;
        }
        if (ioSt.env){
            SQLFreeHandle(SQL_HANDLE_ENV, ioSt.env);
            ioSt.env = nullptr;
        }
    }



#undef SQLCALL

} // namespace odbc


