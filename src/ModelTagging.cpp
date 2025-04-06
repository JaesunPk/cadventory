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
#include <QtConcurrent>
#include <QFuture>
#include "executeCommand.h"

bool ModelTagging::checkOllamaAvailability() {
#ifdef _WIN32
    // Windows command using our blocking function (consider revising this later too)
    std::string output = executeCommandNoWindow("where ollama");
    int result = output.find("ollama") == std::string::npos;
#else
    int result = std::system("which ollama >/dev/null 2>&1");
#endif
    return result == 0;
}

bool ModelTagging::checkModelAvailability(const std::string& modelName) {
#ifdef _WIN32
    std::string output = executeCommandNoWindow("ollama list");
    return output.find(modelName) != std::string::npos;
#else
    std::string command = "ollama list | grep -q \"" + modelName + "\"";
    int result = std::system(command.c_str());
    return result == 0;
#endif
}

ModelTagging::ModelTagging(){
    // instantiate the parser
    parser = ModelParser();
}

ModelTagging::~ModelTagging(){
    // destructor implementation
    if (tagProcess) {
        tagProcess->kill();
        tagProcess->deleteLater();
    }
}

void ModelTagging::generateTags(const std::string& filepath) {
	m_generationCanceled = false; // Reset the cancellation 
    auto blockingTask = [this, filepath]() -> bool {
        // Check if Ollama is available.
        if (!checkOllamaAvailability()) {
            std::cerr << "ERROR: Ollama is not available." << std::endl;
            return false;
        }
        // Check if the model is available.
        if (!checkModelAvailability(this->modelName)) {
            std::cerr << "ERROR: Model " << this->modelName << " is not available." << std::endl;
            return false;
        }
        // Parse metadata.
        ModelMetadata metadata = parser.parseModel(filepath);

        // Build the prompt.
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
        )" << objectPathsStream.str() << R"(

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

        // Write the prompt to a temporary file.
        std::ofstream promptFile("prompt.txt");
        if (!promptFile.is_open()) {
            std::cerr << "ERROR: Could not open prompt.txt for writing." << std::endl;
            return false;
        }
        promptFile << prompt.str();
        promptFile.close();
        return true;
        };

    QFuture<bool> future = QtConcurrent::run(blockingTask);
    QFutureWatcher<bool>* watcher = new QFutureWatcher<bool>(this);
    connect(watcher, &QFutureWatcher<bool>::finished, this, [this, watcher]() {
        bool success = watcher->result();
        watcher->deleteLater();

        if (m_generationCanceled) {
            qDebug() << "Generation was canceled during blocking tasks.";
            emit tagGenerationCanceled();
            return;
        }

        if (!success) {
            emit tagsGenerated({});
            return;
        }

        if (tagProcess) {
            tagProcess->deleteLater();
        }
        tagProcess = new QProcess(this);
        connect(tagProcess, QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished),
            this, &ModelTagging::onTagProcessFinished);

        tagProcess->setProgram("ollama");
        tagProcess->setArguments(QStringList() << "run" << "llama3");
        tagProcess->setStandardInputFile("prompt.txt");
        tagProcess->setStandardOutputFile("temp_tags.txt");

        tagProcess->start();
     });

    watcher->setFuture(future);
}

void ModelTagging::cancelTagGeneration() {
	m_generationCanceled = true;
    if (tagProcess && tagProcess->state() == QProcess::Running) {
        tagProcess->terminate();
        if (!tagProcess->waitForFinished(2000)) {
            tagProcess->kill();
        }
        qDebug() << "Tag generation canceled (process killed).";
        emit tagGenerationCanceled();
    }
    else {
        qDebug() << "Tag generation canceled before process started.";
        emit tagGenerationCanceled();
    }
}

void ModelTagging::onTagProcessFinished(int exitCode, QProcess::ExitStatus exitStatus) {
    Q_UNUSED(exitCode);
    Q_UNUSED(exitStatus);

    std::vector<std::string> tags;
    std::ifstream file("temp_tags.txt");
    std::string content((std::istreambuf_iterator<char>(file)),
        std::istreambuf_iterator<char>());
    file.close();

    std::regex tagPattern(R"((?:^|\n)([A-Za-z]+)(?:\n|$))");
    std::sregex_iterator it(content.begin(), content.end(), tagPattern);
    std::sregex_iterator end;
    int count = 0;
    while (it != end && count < 10) {
        std::string tag = (*it)[1];
        if (!tag.empty()) {
            tags.push_back(tag);
            count++;
        }
        ++it;
    }
    if (tags.size() < 10) {
        std::istringstream contentStream(content);
        std::string line;
        tags.clear();
        while (std::getline(contentStream, line) && tags.size() < 10) {
            line.erase(0, line.find_first_not_of(" \t\n\r\f\v"));
            line.erase(line.find_last_not_of(" \t\n\r\f\v") + 1);
            if (line.empty() || line.find(' ') != std::string::npos || !std::isalpha(line[0])) {
                continue;
            }
            tags.push_back(line);
        }
    }

    std::remove("prompt.txt");
    std::remove("temp_tags.txt");

    // Emit the signal with the generated tags.
    emit tagsGenerated(tags);

    tagProcess->deleteLater();
    tagProcess = nullptr;
}
