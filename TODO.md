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
 - test the whole project - DONE
 - add ability to process multiple queries in one file - DONE
 - fix memory leaks - DONE
 - merge schema and hash table
 - implement primary keys - DONE
 - implement ability to use column reorderings in queries
 - implement transactions - DONE

CONSIDERATIONS:
 - readNode and readPage need to propagate failure
 - Changing query column order is not supported
 - Stripe allocation is incorrect - 1:1 ratio instead of 1:M ratio
 - B tree merges require loading all child pages to adjust their parent members. Do child pages need to have parent members? Maybe they can have them in memory but not on disk. Would be a large performance boost if so.