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
#include <string>
#include <vector>

namespace odbc {
   
    class Connection{

    private:
        SQLHENV _env = nullptr;
        SQLHDBC _hDb = nullptr;
        
        std::string _lastError;
        
        bool _isConnect = false;

    public:

        Connection(const std::string& dbServer, const std::string& user = "", const std::string& password = ""){

#define SQLCALL_EXT return;  
#define SQLCALL(ht, h, f, errMess){                                                                     \
                 SQLRETURN ret = f;                                                                     \
                 if ((ret != SQL_SUCCESS) && (ret != SQL_SUCCESS_WITH_INFO)){                           \
                    char		stat[10]{0};                                                            \
                    SQLINTEGER	err;                                                                    \
                    SQLSMALLINT	mlen;                                                                   \
                    char        msg[1024]{0};                                                           \
                    SQLGetDiagRec(ht, h, 1, (SQLCHAR*)stat, &err, (SQLCHAR*)msg, 1024, &mlen);          \
                    _lastError = errMess + std::string(" err ") + std::to_string(err) + " mess " + msg; \
                    SQLCALL_EXT                                                                         \
                 };                                                                                     \
               }

            // allocate environment handle
            SQLCALL(SQL_HANDLE_ENV, _env,
                    SQLAllocHandle(SQL_HANDLE_ENV, SQL_NULL_HANDLE, &_env), "connect: environment SQLAllocHandle");

            // register version 
            SQLCALL(SQL_HANDLE_ENV, _env,
                    SQLSetEnvAttr(_env, SQL_ATTR_ODBC_VERSION, (void*)SQL_OV_ODBC3, 0), "connect: SQLSetEnvAttr");

            // allocate connection handle
            SQLCALL(SQL_HANDLE_DBC, _hDb,
                    SQLAllocHandle(SQL_HANDLE_DBC, _env, &_hDb), "connect: connection SQLAllocHandle");

            // set connect timeout
            SQLCALL(SQL_HANDLE_DBC, _hDb,
                    SQLSetConnectAttr(_hDb, SQL_LOGIN_TIMEOUT, (SQLPOINTER *)5, 0), "connect: SQLSetConnectAttr");

            // connect 
            SQLCALL(SQL_HANDLE_DBC, _hDb,
                    SQLConnect(_hDb, (SQLCHAR*)dbServer.c_str(), SQL_NTS,
                                     (SQLCHAR*)user.c_str(), SQL_NTS,
                                     (SQLCHAR*)password.c_str(), SQL_NTS), "connect: SQLConnect");
         
            _isConnect = _env && _hDb;
        }

        ~Connection(){

            if (_hDb){
                SQLDisconnect(_hDb);
                SQLFreeHandle(SQL_HANDLE_DBC, _hDb);
            }
            if (_env){
                SQLFreeHandle(SQL_HANDLE_ENV, _env);
            }
        }

        bool isConnect(){

            return _isConnect;
        }
        
        std::string getLastError(){

            return _lastError;
        }

        template<typename T>
        bool setOption(SQLINTEGER optnum, const T& value){
            
            if (!_isConnect) return false;

#undef SQLCALL_EXT         
#define SQLCALL_EXT return false;      
                        
            SQLCALL(SQL_HANDLE_DBC, _hDb, 
                    SQLSetConnectAttr(_hDb, optnum, (SQLPOINTER)value, 0), "setOption: SQLSetConnectAttr");

            return true;
        }

        template<typename T>
        bool getOption(SQLINTEGER optnum, T& out){
                        
            if (!_isConnect) return false;
            
            SQLINTEGER optSize = 0;

            SQLCALL(SQL_HANDLE_DBC, _hDb, 
                    SQLGetConnectAttr(_hDb, optnum, (SQLPOINTER)&out, 0, &optSize), "getOption: SQLGetConnectAttr");
                        
            return true;
        }

        bool query(const std::string& query, std::vector<std::vector<std::string>>& results){

            if (!_isConnect) return false;
        
#undef SQLCALL_EXT         
#define SQLCALL_EXT if (hStmt) SQLFreeHandle(SQL_HANDLE_STMT, hStmt);  \
                    return false;   


            SQLHSTMT hStmt = nullptr;
            SQLCALL(SQL_HANDLE_DBC, _hDb,
                    SQLAllocHandle(SQL_HANDLE_STMT, _hDb, &hStmt), "query: SQLAllocHandle");

            SQLCALL(SQL_HANDLE_STMT, hStmt,
                    SQLExecDirect(hStmt, (SQLCHAR*)query.c_str(), int(query.size())), "query: SQLExecDirect");
                                
            SQLLEN rows = 0;
            SQLCALL(SQL_HANDLE_STMT, hStmt, 
                    SQLRowCount(hStmt, &rows), "query: SQLRowCount");
                  
            // rows affected by an UPDATE, INSERT, or DELETE
            if (rows >= 0){
                SQLFreeHandle(SQL_HANDLE_STMT, hStmt);
                return true;
            }

            SQLSMALLINT cols = 0;
            SQLCALL(SQL_HANDLE_STMT, hStmt,
                    SQLNumResultCols(hStmt, &cols), "query: SQLNumResultCols");

            std::vector<std::string> colVals(cols);
            for (int i = 0; i < cols; ++i){

                std::string& cval = colVals[i];

                SQLLEN  ssType;
                SQLCALL(SQL_HANDLE_STMT, hStmt, 
                        SQLColAttribute(hStmt,
                                        i + 1,
                                        SQL_DESC_DISPLAY_SIZE,
                                        NULL,
                                        0,
                                        NULL,
                                        &ssType), "query: SQLColAttribute");

                cval.resize(ssType);

                SQLCALL(SQL_HANDLE_STMT, hStmt, 
                        SQLBindCol(hStmt,
                                   i + 1,
                                   SQL_C_CHAR,
                                   (SQLPOINTER)cval.c_str(),
                                   ssType,
                                   &ssType), "query: SQLBindCol");
            }
            
            if (!results.empty())
                results.clear();

            SQLRETURN ret = SQLFetch(hStmt);
            while (ret != SQL_NO_DATA){
                
                results.push_back(colVals);

                ret = SQLFetch(hStmt);
            }
                  
            SQLFreeHandle(SQL_HANDLE_STMT, hStmt);

            return true;  
        }
    };
    
#undef SQLCALL_EXT  
#undef SQLCALL   
} // namespace odbc


