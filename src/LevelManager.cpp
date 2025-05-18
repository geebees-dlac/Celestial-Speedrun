#include "LevelManager.hpp"

using namespace std;
using namespace rapidjson;

rapidjson::Document lvldata;

rapidjson::Document* readFile(){
    FILE* fp = fopen("../assets/levels/level1.json", "r"); //hardcode placeholder

    char readBuffer[65536];
    rapidjson::FileReadStream is(fp, readBuffer, sizeof(readBuffer));

    rapidjson::Document* d = new rapidjson::Document();
    d->ParseStream(is);

    fclose(fp);

    if (d->HasParseError()){
        std::cerr << "Error parsing file" << std::endl;
        exit(1);
    }

    std::cout << "LOADING Level " << (*d)["lvlcode"].GetInt() << std::endl;

    return d;
}

int savetest(rapidjson::Document* d){
    std::cout << (*d)["test"].GetString() << std::endl;

    if ((*d).HasParseError()){
        std::cerr << "Error parsing file" << std::endl;
        exit(1);
    }

    return 0;
}

void freeData(rapidjson::Document* d){
    delete d;
}