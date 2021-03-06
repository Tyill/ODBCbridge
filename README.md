# ODBCbridge

Simple ODBC client library [one header only](https://github.com/Tyill/ODBCbridge/blob/master/include).

## Example of use

```cpp
#include "../include/ODBCbridge.h"

using namespace std;

int main(int argc, char* argv[]){

    odbc::Connection db("testserv");
    
    if (db.isConnect()){
        printf("db connect ok\n");
    }else{
        printf("db connect error: %s\n", db.getLastError().c_str());
        return -1;
    }
        
    vector<vector<string>> results;
    if (db.query("SELECT * FROM tblColorMap;", results)){
        printf("db query ok, results rows: %d\n", results.size());
    }else{
        printf("db query error: %s\n", db.getLastError().c_str());
        return -1;
    }

    return 0;
}
```

## License
Licensed under an [MIT-2.0]-[license](LICENSE).