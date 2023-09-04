#include <cstring>
#include <string>

#include "IniParser.h"

IniParser::IniParser(){
	state = 0;
	curSection = "";
	curKey = "";
	curValue = "";
}

bool IniParser::doStep(char c){
	if(state == 0){//state for line beginning
		curKey = ""; curValue = "";
		if(c == ';' || c == '#'){state = 1; return false;}//comments
		if(c == '['){curSection = ""; state = 2; return false;}//new section
		if(c == ' ' || c == '\t' || c == '\n' || c == '\r'){return false;}//ignore whitespaces
		curKey.append(1, c); state = 3; return false;//else*
	}else if(state == 1){//state for comment
		if(c == '\n'){state = 0; return false;}//new line
		return false;//else
	}else if(state == 2){//state for section
		if(c == ']'){state = 1; return false;}//after ] everything is treated as comment
		curSection.append(1, c); return false;//else*
	}else if(state == 3){//state for key start
		if(c == '='){//read value and remove potential whitespaces from key
			int i = curKey.size()-1;
			for(; i>=0; i--){
				char l = curKey[i];
				if(l != ' ' && l != '\t'){break;}
			}
			if(i>=0){curKey = curKey.substr(0, i+1);}
			state = 4; return false;
		}else{curKey.append(1, c); return false;}//else*
	}else if(state == 4){//state after =
		if(c == ' ' || c == '\t' || c == '\r'){return false;}//ignore whitespace
		if(c == '\n'){state = 0; return true;}//new line
		curValue.append(1, c); state = 5; return false;//else*
	}else if(state == 5){
		if(c == '\n'){state = 0; return true;}//new line
		if(c != '\r'){curValue.append(1, c);} return false;//else*
	}
	return false;	
}
