# NexusDB
NexusDB is a sophisticated multi-model database system designed to support both relational and document-based data models. It aims to provide a flexible, high-performance solution for modern data management needs.

## Features

- Dual support for relational and document-based data models
- High-performance C++ core engine
- Python Flask application layer for easy integration
- Web-based management interface
- ACID compliance
- Advanced indexing and query optimization

## Project Structure
- `core/`: C/C++ core database engine
- `app/`: Python Flask application layer
- `web/`: Web interface
- `drivers/`: JDBC and ODBC drivers
- `docs/`: Project documentation
- `tests/`: Project-wide tests

Note: Common scripts and resources are located in the separate NexusDB-Common repository.

## Getting Started

### Prerequisites

1. CMake 3.10 or higher
2. C++17 compatible compiler
3. Python 3.7 or higher
4. Node.js and npm (for web interface)

### Building the Core Engine

1. Clone the repository:
```bash
git clone https://github.com/sahild91/NexusDB.git
cd NexusDB
```

2. Build the project:
```bash
mkdir build
cd build
cmake ..
cmake --build .
```

3. Install:
```bash
cmake --install .
```

### Setting up the Python Application

1. Navigate to the app directory:
```bash
cd ../app
```

2. Create a virtual environment and activate it:
```bash
python -m venv venv
source venv/bin/activate  # On Windows, use `venv\Scripts\activate`
```

3. Install dependencies:
```bash
pip install -r requirements.txt
```

4. Run the Flask application:
```bash
flask run
```


### Setting up the Web Interface

1. Navigate to the web directory:
```bash
cd ../web
```

2. Install dependencies:
```bash
npm install
```

3. Start the development server:
```bash
npm start
```


## Documentation
For more detailed information, please refer to the `docs/` directory or visit our online documentation.

## Contributing
We welcome contributions to NexusDB! Please see our Contributing Guide for more details on how to get started.

## License
NexusDB is licensed under the GNU General Public License v3.0. See the `LICENSE` file in the project root for the full license text.

## Contact
For questions, issues, or support, please open an issue on our GitHub[https://www.github.com/sahild91/NexusDB.git] repository or contact our development team at `sahildiwakar91@gmail.com`