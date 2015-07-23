#include "options/options.h"
#include "parser/parser.h"
#include "parser/parser_builder.h"
#include "expr/expr_manager.h"
#include "expr/command.h"
#include "main/main.h"
#include "main/command_executor.h"

#include <string>
#include <iostream>

using namespace std;
using namespace CVC4;
using namespace CVC4::parser;
using namespace CVC4::main;

const string FILE_OR_INPUT_DELIMETER = "---FINISHED---";
const string STDIN_FILENAME = "<stdin>";

enum ReadLineResult {
  STREAM_COMPLETE,
  FILE_COMPLETE,
  MORE_TO_FILE
};

class CVC4Session {
public:
  CVC4Session(Parser* parser, CommandExecutor* executor, Options& options)
    : options(options), parser(parser), executor(executor) {}

  // assumes the line is well-formed
  void parse_and_execute_line(string& line) {
    parser->setInput(Input::newStringInput(options[options::inputLanguage],
					   line,
					   STDIN_FILENAME));
    Command* command = parser->nextCommand();
    //pExecutor = executor;
    executor->doCommand(command);
    delete command;
  }

  ~CVC4Session() {
    delete parser;
    delete executor;
  }

private:
  Options& options;
  Parser* parser;
  CommandExecutor* executor;
};

class CVC4SessionFactory {
public:
  CVC4SessionFactory(Options& options, ExprManager& exprManager)
    : options(options), exprManager(exprManager) {
    parserBuilder = new ParserBuilder(&exprManager, STDIN_FILENAME);
    parserBuilder->withOptions(options);
    parserBuilder->withStringInput("");
  }

  CVC4Session* newSession() {
    return new CVC4Session(parserBuilder->build(),
			   new CommandExecutor(exprManager, options),
			   options);
  }

  ~CVC4SessionFactory() {
    delete parserBuilder;
  }

private:
  Options& options;
  ExprManager& exprManager;
  ParserBuilder* parserBuilder;
};

ReadLineResult read_line(CVC4Session* session,
			 istream& input) {
  string line;

  if (getline(input, line)) {
    if (line.compare(FILE_OR_INPUT_DELIMETER) == 0) {
      return FILE_COMPLETE;
    } else {
      session->parse_and_execute_line(line);
      cout << FILE_OR_INPUT_DELIMETER << endl;
      return MORE_TO_FILE;
    }
  } else {
    return STREAM_COMPLETE;
  }
}

// returns true if we have more files, else false
bool read_file_incrementally(CVC4SessionFactory& sessionFactory,
			     istream& input) {
  CVC4Session* session = sessionFactory.newSession();

  // loop until we hit a terminator of some sort
  while (true) {
    switch (read_line(session, input)) {
    case STREAM_COMPLETE:
      delete session;
      return false;
    case FILE_COMPLETE:
      delete session;
      return true;
    case MORE_TO_FILE:
      break;
    }
  }
}

void read_files(CVC4SessionFactory& sessionFactory,
		istream& input) {
  while (read_file_incrementally(sessionFactory, input)) {}
}

int main(int argc, char** argv) {
  Options options;
  options.parseOptions(argc, argv);
  options.set(options::inputLanguage, language::input::LANG_SMTLIB_V2_0);
  options.set(options::outputLanguage, language::output::LANG_SMTLIB_V2_0);

  //pOptions = &options;
  //cvc4_init();
  ExprManager exprManager(options);
  CVC4SessionFactory sessionFactory(options, exprManager);
  read_files(sessionFactory, cin);

  //delete options;
  return 0;
}

