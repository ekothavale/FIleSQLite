TODO:
 - implement hashtables - DONE
	- mapping table names to column names and schema files - DONE
 - implement separate hashing function to map pks to internal
   keys (pagenum | offset) - DONE
 - implement scanner opcodes - DONE
 - implement logical opcodes - DONE
 - implement control flow opcodes - DONE
 - implement database manipulation opcodes - DONE
 - implement database definition opcodes - DONE
 - write compiler second pass to generate code from AST - DONE
 - test every component of the front end - DONE
 - test the whole project

CONSIDERATIONS:
 - The storage engine doesn't use any of the schema to customize the table
 - readNode and readPage need to propagate failure
 - Entering a blank line into REPL causes segfault
 - Primary keys are not supported
 - Changing query column order is not supported