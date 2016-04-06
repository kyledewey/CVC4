/*********************                                                        */
/*! \file main.cpp
 ** \verbatim
 ** Top contributors (to current version):
 **   Morgan Deters, Tim King, Christopher L. Conway
 ** This file is part of the CVC4 project.
 ** Copyright (c) 2009-2016 by the authors listed in the file AUTHORS
 ** in the top-level source directory) and their institutional affiliations.
 ** All rights reserved.  See the file COPYING in the top-level source
 ** directory for licensing information.\endverbatim
 **
 ** \brief Main driver for CVC4 executable
 **
 ** Main driver for CVC4 executable.
 **/
#include "main/main.h"

#include <cstdlib>
#include <cstring>
#include <fstream>
#include <iostream>
#include <stdio.h>
#include <unistd.h>

#include "base/configuration.h"
#include "base/output.h"
#include "expr/expr_manager.h"
#include "main/command_executor.h"
#include "main/interactive_shell.h"
#include "options/language.h"
#include "options/options.h"
#include "parser/parser.h"
#include "parser/parser_builder.h"
#include "parser/parser_exception.h"
#include "smt/command.h"
#include "smt/smt_engine.h"
#include "util/result.h"
#include "util/statistics.h"
#include "options/options.h"
#include "parser/parser.h"
#include "parser/parser_builder.h"
#include "expr/expr_manager.h"
#include "main/main.h"
#include "main/command_executor.h"
#include "base/exception.h"

#include <string>
#include <iostream>

using namespace std;
using namespace CVC4;
using namespace CVC4::parser;
using namespace CVC4::main;

const string FILE_OR_INPUT_DELIMETER = "---FINISHED---";
const string STDIN_FILENAME = "<stdin>";
const string ERROR_DETECTED = "---ERROR DETECTED---";
const string READY = "---READY---";

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
    parser->setInput(Input::newStringInput(options.getInputLanguage(),
					   line,
					   STDIN_FILENAME));
    Command* command = NULL;

    try {
      command = parser->nextCommand();
      if (!command) {
	cout << endl << ERROR_DETECTED << endl;
      } else {
	if (!executor->doCommand(command)) {
	  cout << endl << ERROR_DETECTED << endl;
	}
      }
    } catch (Exception& exp) {
      cout << endl << exp.what() << endl << ERROR_DETECTED << endl;
    }

    if (command) {
      delete command;
    }
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
  CVC4SessionFactory(Options& options)
    : options(options) {
    exprManager = new ExprManager(options);
    parserBuilder = new ParserBuilder(exprManager, STDIN_FILENAME);
    parserBuilder->withOptions(options);
    parserBuilder->withStringInput("");
  }

  CVC4Session* newSession() {
    return new CVC4Session(parserBuilder->build(),
			   new CommandExecutor(*exprManager, options),
			   options);
  }

  ~CVC4SessionFactory() {
    delete parserBuilder;
    delete exprManager;
  }

private:
  Options& options;
  ExprManager* exprManager;
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
  while (read_file_incrementally(sessionFactory, input)) {
    cout << endl << READY << endl;
  }
}

int main(int argc, char** argv) {
  Options options;
  options.parseOptions(argc, argv);
  options.setInputLanguage(language::input::LANG_SMTLIB_V2_0);
  options.setOutputLanguage(language::output::LANG_SMTLIB_V2_0);

  //pOptions = &options;
  //cvc4_init();
  //ExprManager exprManager(options);
  CVC4SessionFactory sessionFactory(options);
  read_files(sessionFactory, cin);

  //delete options;
  return 0;
}

