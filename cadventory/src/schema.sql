- SQLite database schema for CADventory

-- Table for storing primary model information and overrides
CREATE TABLE IF NOT EXISTS models (
    id INTEGER PRIMARY KEY AUTOINCREMENT,
    short_name TEXT NOT NULL, -- The folder name which is unique for each model
    primary_cad_file TEXT, -- The path to the primary CAD file, can be relative to the short_name folder
    override_info TEXT -- A JSON or text format to hold any overrides or additional information
);

-- Additional model info is stored in the model folder as metadata.json
