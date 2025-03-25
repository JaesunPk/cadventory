#include <iostream>
#include <string>
#include <sstream>
#include <iostream>
#include <vector>
#include <cstdlib>
#include <fstream>
#include <regex>
#include "ModelTagging.h"

bool ModelTagging::checkOllamaAvailability() {
    #ifdef _WIN32
        // Windows command
        int result = std::system("where ollama >nul 2>nul");
    #else
        // Linux/Mac command
        int result = std::system("which ollama >/dev/null 2>&1");
    #endif
    
    return result == 0;
}

bool ModelTagging::checkModelAvailability(const std::string& modelName) {
    #ifdef _WIN32
        // Windows command (redirect to null)
        std::string command = "ollama list | findstr \"" + modelName + "\" >nul 2>nul";
    #else
        // Linux/Mac command
        std::string command = "ollama list | grep -q \"" + modelName + "\"";
    #endif
    
    int result = std::system(command.c_str());
    return result == 0;
}

ModelTagging::ModelTagging(){
    // instantiate the parser
    parser = ModelParser();
}

ModelTagging::~ModelTagging(){
    // destructor implementation
}

std::vector<std::string> ModelTagging::generateTags(std::string filepath) {
    std::vector<std::string> tags = {};

    // check if ollama is available
    if(!checkOllamaAvailability()){
        std::cerr << "ERROR: Ollama is not available." << std::endl;
        return tags;
    }

    // check if the model is available
    if(!checkModelAvailability(this->modelName)){
        std::cerr << "ERROR: Model " << this->modelName << " is not available." << std::endl;
        return tags;
    }

    ModelMetadata metadata = parser.parseModel(filepath);
    std::ostringstream objectPathsStream;

    for (const auto& object : metadata.objectFiles) {
        objectPathsStream << "  - " << object << "\n";
    }

    std::ostringstream prompt;
    prompt << R"(
        You are an expert CAD modeler organizing CAD files for an engineering database.
        
        Generate exactly 10 relevant tags for categorization and search filtering of this 3D CAD model.
        
        ### **File Metadata:**
        - **Filepath:** )" << filepath << R"(
        - **Title:** )" << metadata.title << R"(
        - **Object Paths:**  
    )" 
        << objectPathsStream.str() << R"(

        ### **Instructions:**
        - **ONLY use words directly related to the object in the CAD model.**  
        - **Do NOT include metadata like names, file paths, BRL-CAD, or dates.**  
        - **Prioritize useful search terms for categorization (e.g., mechanical, architectural, vehicle, tool).**  
        - **Infer object identity from file names, object names, and folder structure.**  
        - **Do NOT make them generic (e.g., object, model, 3D).**
        - **Do NOT assume additional details beyond what's in the metadata.**
        - **Your output must be EXACTLY 10 single-word tags, one per line with no numbering or additional formatting.**

        ### Example:
        For a teapot model, appropriate tags would be:
        Teapot
        Container
        Lid
        Ceramic
        Tableware
        Kitchenware
        Beverage
        Drinking
        Serving
        Decorative
    )";

    // Save the prompt to a temp file for execution
    std::ofstream promptFile("prompt.txt");
    promptFile << prompt.str();
    promptFile.close();

    // Construct the command for Ollama to call LLaMA
    std::string command = "ollama run llama3 < prompt.txt > temp_tags.txt";
    int result = std::system(command.c_str());

    if (result == 0) {
        // Clear previous tags
        tags.clear();
        
        // Read the output from temp_tags.txt
        std::ifstream file("temp_tags.txt");
        std::string content((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
        file.close();
        
        // Parse the tags from the output using regex to find single words
        // This handles various potential formats that the LLM might respond with
        std::regex tagPattern(R"((?:^|\n)([A-Za-z]+)(?:\n|$))");
        std::sregex_iterator it(content.begin(), content.end(), tagPattern);
        std::sregex_iterator end;
        
        // Extract tags, taking at most 10
        int count = 0;
        while (it != end && count < 10) {
            std::string tag = (*it)[1];
            if (!tag.empty()) {
                tags.push_back(tag);
                count++;
            }
            ++it;
        }
        
        // If we didn't get enough tags, try a simpler parsing approach
        if (tags.size() < 10) {
            // Alternative parsing for different output formats
            std::istringstream contentStream(content);
            std::string line;
            tags.clear();
            
            while (std::getline(contentStream, line) && tags.size() < 10) {
                // Trim whitespace
                line.erase(0, line.find_first_not_of(" \t\n\r\f\v"));
                line.erase(line.find_last_not_of(" \t\n\r\f\v") + 1);
                
                // Skip empty lines and lines with non-tag content
                if (line.empty() || line.find(' ') != std::string::npos || 
                    !std::isalpha(line[0])) {
                    continue;
                }
                
                tags.push_back(line);
            }
        }
        
        // Print the tags for verification
        // std::cout << "\nGenerated " << tags.size() << " tags for: " << metadata.title << std::endl;
        // std::cout << "-------------------------------------" << std::endl;
        // for (const auto& tag : tags) {
        //     std::cout << "- " << tag << std::endl;
        // }
        // std::cout << "-------------------------------------" << std::endl;
    }
    else {
        std::cerr << "ERROR: LLaMA call failed." << std::endl;
    }

	// Remove the temp files
	std::remove("prompt.txt");
	std::remove("temp_tags.txt");

	return tags;
}
