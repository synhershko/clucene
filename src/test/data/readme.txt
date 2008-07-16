Reuters-21578 is the data set we use to test for index compatibility.

Retuers-21578-index directory contains an index created using java lucene 
version 1.4.3. The following changes were made to the java demo to create 
this index:

* The FileDocument is using f.getName() instead of f.getPath() for the path field.
* The modified field was removed
* Used a special analyser instead of StandardAnalyzer. This is because the text 
  classification of java differs to that of clucene. See the TestReuters.cpp code
  for implementation. Java version used exactly the same implementation.
