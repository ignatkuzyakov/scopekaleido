#### About
This implementation of LLVM Kaleidoscope utilizes Flex and Bison for the lexer and parser.

#### Grammar
```
  top:            definition SCOLON         
                | external   SCOLON                                     
                | toplvlexpr SCOLON         
                | QUIT                      

  external:       EXTERN prototype          

  definition:     DEF prototype expr        

  prototype:      ID LBR protargs RBR       
      
  protargs:       /*empty*/                    
                | protargs COMMA ID         
                | ID                        

  toplvlexpr:     expr                       
                                                                                        
  call:           ID LBR callargs RBR       

  callargs:      /*empty*/                  
                | callargs COMMA expr       
                | expr                       

  expr:           expr PLUS  term           
                | expr MINUS term           
                | term                      
  
  term:           term MUL factor           
                | term DIV factor          
                | factor                  
                | call                     

  factor:         LBR expr RBR            
                | NUMBER                  
                | ID                       
```
#### How to build
```
mkdir build && cd build
cmake .. -DCMAKE_BUILD_TYPE="Release"
make
```
