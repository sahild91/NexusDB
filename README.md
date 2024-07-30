# NexusDB
NexusDB is a sophisticated multi-model database system designed to support both relational and document-based data.

## Project Structure
- `core/`: C/C++ core database engine
- `app/`: Python Flask application layer
- `web/`: Web interface
- `drivers/`: JDBC and ODBC drivers
- `docs/`: Project documentation
- `tests/`: Project-wide tests

Note: Common scripts and resources are located in the separate NexusDB-Common repository.

## Getting Started
Follow these steps to set up your development environment for NexusDB:

1. Clone the repository:
```bash
git clone https://github.com/your-username/NexusDB-Main.git
cd NexusDB-Main
```

2. Set up the submodule for common resources:
```bash
git submodule update --init --recursive
```

3. Install dependencies:

    - For the core C/C++ engine:
```bash
    cd core
    ./configure
    make
```

    - For the Python Flask application:
```bash
    cd app
    python -m venv venv
    source venv/bin/activate  # On Windows use `venv\Scripts\activate`
    pip install -r requirements.txt
```

    - For the web interface:
```bash
    cd web
    npm install
```

4. Set up the database:
```bash
cd core
./setup_database.sh
```

5. Run the development servers:

    - For the core engine:
```bash
    cd core
    ./run_engine.sh
```

    - For the Flask app:
```bash
    cd app
    flask run
```

    - For the web interface:
```bash
    cd web
    npm start
```

6. Access the web interface at `http://localhost:3000` and the API at `http://localhost:5000`.

For more detailed instructions, please refer to the `docs/getting_started.md` file.

## Contributing

We welcome contributions to NexusDB! Here's how you can contribute:

1. Fork the repository.
2. Create a new branch for your feature or bug fix.
3. Make your changes, ensuring you follow our coding standards (see docs/coding_standards.md).
4. Write or update tests for your changes.
5. Run the test suite to ensure all tests pass.
6. Commit your changes and push to your fork.
7. Create a pull request with a clear description of your changes.

Please read our `CONTRIBUTING.md` file for more detailed information on our contribution process, coding standards, and community guidelines.

## License
NexusDB is licensed under the GNU General Public License v3.0. See the `LICENSE` file in the project root for the full license text.

## Contact
For questions, issues, or support, please open an issue on our GitHub[https://www.github.com/sahild91/NexusDB.git] repository or contact our development team at `sahildiwakar91@gmail.com`