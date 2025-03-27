#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <Windows.h>
#include <shellapi.h>
#pragma comment(lib, "Shell32.lib")
#endif

#include <iostream>
#include <string>
#include <sstream>
#include <vector>
#include <cstdlib>
#include <fstream>
#include <regex>
#include "ModelTagging.h"
#include <QProcess>
#include <QDebug>

bool ModelTagging::checkOllamaAvailability() {
    QProcess process;

#ifdef _WIN32
    // Set creation flags to prevent showing console window
    process.setCreateProcessArgumentsModifier([](QProcess::CreateProcessArguments* args) {
        args->flags |= CREATE_NO_WINDOW;
        });
    process.start("where", QStringList() << "ollama");
#else
    process.start("which", QStringList() << "ollama");
#endif

    process.waitForFinished();
    return process.exitCode() == 0;
}

bool ModelTagging::checkModelAvailability(const std::string& modelName) {
    QProcess process;

#ifdef _WIN32
    // Set creation flags to prevent showing console window
    process.setCreateProcessArgumentsModifier([](QProcess::CreateProcessArguments* args) {
        args->flags |= CREATE_NO_WINDOW;
        });

    // Get the full list of models first
    process.start("cmd", QStringList() << "/c" << "ollama list");
    process.waitForFinished();

    if (process.exitCode() != 0) {
        qDebug() << "Ollama list command failed with exit code:" << process.exitCode();
        return false;
    }

    // Read the output and check if the model name exists in it
    QString output = process.readAllStandardOutput();
    qDebug() << "Ollama available models:" << output;

    return output.contains(QString::fromStdString(modelName));
#else
    process.start("sh", QStringList() << "-c" << QString::fromStdString("ollama list | grep -q \"" + modelName + "\""));
    process.waitForFinished();
    return process.exitCode() == 0;
#endif
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

    bool success = false;

#ifdef _WIN32
    // Create a batch file that will run the command without showing the window
    std::ofstream batchFile("run_ollama.bat");
    batchFile << "@echo off\r\n";
    batchFile << "ollama run llama3 < prompt.txt > temp_tags.txt\r\n";
    batchFile << "exit /b %ERRORLEVEL%\r\n";
    batchFile.close();

    // Convert to wide characters for Windows API
    std::wstring wBatchFile = L"run_ollama.bat";
    std::wstring wParameters = L"";

    // Use ShellExecuteEx to run the batch file without showing a window
    SHELLEXECUTEINFO shExInfo = { 0 };
    shExInfo.cbSize = sizeof(shExInfo);
    shExInfo.fMask = SEE_MASK_NOCLOSEPROCESS;
    shExInfo.hwnd = NULL;
    shExInfo.lpVerb = NULL;
    shExInfo.lpFile = wBatchFile.c_str();
    shExInfo.lpParameters = wParameters.c_str();
    shExInfo.lpDirectory = NULL;
    shExInfo.nShow = SW_HIDE;
    shExInfo.hInstApp = NULL;

    if (ShellExecuteEx(&shExInfo)) {
        WaitForSingleObject(shExInfo.hProcess, INFINITE);
        DWORD exitCode;
        GetExitCodeProcess(shExInfo.hProcess, &exitCode);
        CloseHandle(shExInfo.hProcess);
        success = (exitCode == 0);
    }

    // Delete the batch file
    std::remove("run_ollama.bat");
#else
    // Use process on non-Windows platforms
    QProcess process;
    process.start("sh", QStringList() << "-c" << "ollama run llama3 < prompt.txt > temp_tags.txt");
    process.waitForFinished(-1);
    success = (process.exitCode() == 0);
#endif

    if (success) {
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
