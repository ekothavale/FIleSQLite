TODO:
 - implement hashtables - DONE
	- mapping table names to column names and schema files - DONE
 - implement separate hashing function to map pks to internal
   keys (pagenum | offset) - DONE
 - implement scanner opcodes - DONE
 - implement logical opcodes - DONE
 - implement control flow opcodes - DONE
 - implement database management opcodes
 - implement database definition opcodes
 - write compiler second pass to generate code from AST
 - test every component of the front end
 - test the whole project

CONSIDERATIONS:
 - Potential issue where tables are stored on disk as hashes but table file names expect a string (originally intended to be the table name)
	- could use hash as table file name 
 - VM needs to receive table from compiler since the compiler may need to populate the table further at compile time