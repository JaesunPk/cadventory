#ifndef MODELPARSER_H
#define MODELPARSER_H

#include <string>
#include <vector>
#include "ModelMetadata.h" 

class ModelParser {
public:
    ModelParser();

    // parse the model file and return the metadata
    ModelMetadata parseModel(std::string filepath);
};

#endif // MODELPARSER_H