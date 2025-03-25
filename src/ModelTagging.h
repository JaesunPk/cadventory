#ifndef MODELTAGGING_H
#define MODELTAGGING_H

#include <string>
#include <vector>
#include "ModelParser.h"

/**
 * @brief main class for handling model tags.
 * 
 * This class will generate a list of tags for a model after given a set prompt.abs
 * The prompt will consist of the file path to the model itself, the title, and the object files that are used to generate the model.
 * This is known as the model's metadata. The extraction of model data will be through the ModelParser class.
 * 
 * How to use:
 * - simply create and instance of the class and call generateTags(std::string filepath) to generate the tags.
 */
class ModelTagging {
public:
    ModelTagging();

    ~ModelTagging();

    void generateTags(std::string filepath);
    std::vector<std::string> getTags();

private:
    bool checkOllamaAvailability();
    bool checkModelAvailability(const std::string& modelName);
    std::string modelName = "llama3"; // default model name - change if needed
    std::vector<std::string> tags;
    ModelParser parser;
};

#endif // MODELTAGGING_H