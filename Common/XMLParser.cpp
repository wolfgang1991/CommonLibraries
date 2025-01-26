#include <StringHelpers.h>
#include <XMLParser.h>

#include <fstream>

XMLTag::XMLTag(XMLTag* parent){
	this->parent = parent;
}

std::string XMLTag::getInheritance(){
	if(parent==NULL){
		return std::string(name);
	}else{
		return std::string(name).append("->").append(parent->getInheritance());
	}
}

void XMLTag::printInheritance(){
	for(auto it=attributes.begin(); it!=attributes.end(); ++it){
		printf("%s=%s ", it->first.c_str(), it->second.c_str());
	}
	if(parent==NULL){
		printf("%s\n", name.c_str());
	}else{
		printf("%s -> ", name.c_str());
		parent->printInheritance();
	}
}

XMLParser::XMLParser(IParsingCallback* callback){
	dom = NULL;
	mode = 0;
	cbk = callback;
	line = 0;
	charFilter = new IIntermediateCharacterFilter();
}

XMLParser::~XMLParser(){
	delete charFilter;
	delete dom;//NULL also fine
}

void XMLParser::setIntermediateCharacterFilter(IIntermediateCharacterFilter* charFilter){
	if(charFilter){
		delete this->charFilter;
		this->charFilter = charFilter;
	}
}

int XMLParser::getCurrentLine(){
	return line;
}

void XMLParser::parse(char c){
	if(c=='\n'){line++;}
	if(mode==0){
		if(c=='<'){
			mode = 1;
			dom = new XMLTag(dom);
		}else if(dom && charFilter->useIntermediateChar(c)){
			dom->intermediate << c;
		}
	}else if(mode==1){
		if(c=='/'){
			mode = 8;
		}else if(c=='?'){
			mode = 10;
		}else if(c=='!'){
			mode = 12;
		}else if(!isWhitespace(c)){
			mode = 2;
			token.reset(); token << c;
		}
	}else if(mode==2){
		if(c=='>'){
			dom->name = token.str();
			mode = 0;
			cbk->OnOpenTag(this);
		}else if(c=='/'){
			mode = 7;
		}else if(isWhitespace(c)){
			mode = 3;
			dom->name = token.str();
		}else{
			token << c;
		}
	}else if(mode==3){
		if(c=='>'){
			mode = 0;
			cbk->OnOpenTag(this);
		}else if(c=='/'){
			mode = 7;
		}else if(!isWhitespace(c)){
			mode = 4;
			token.reset(); token << c;
		}
	}else if(mode==4){
		if(c=='='){
			//ckey = token.str();
			valueDestination = &(dom->attributes[token.str()]);//avoid extra key copy for more speed
			mode = 5;
		}else if(!isWhitespace(c)){
			token << c;
		}
	}else if(mode==5){
		if(c=='"'){
			mode = 6;
			token.reset();
		}
	}else if(mode==6){
		if(c=='"'){
			//dom->attributes[ckey] = token.str();//dom->attributes.emplace(ckey, token.str());// // emplace is slower than =
			*valueDestination = token.str();
			mode = 3;
		}else{
			token << c;
		}
	}else if(mode==7){
		if(c=='>'){
			mode = 0;
			cbk->OnOpenTag(this);
			cbk->OnCloseTag(this);
			domBack();
		}else if(!isWhitespace(c)){
			mode = 4;
			token.reset(); token << c;
		}
	}else if(mode==8){
		if(!isWhitespace(c)){
			token.reset(); token << c;
			mode = 9;
		}
	}else if(mode==9){
		if(c=='>'){
			domBack();//because of extra closing tag
			if(dom!=NULL){
				if(token.str() != dom->name){
					printf("WARNING: Closing tag </%s> closes <%s> line: %i\n  See inheritance: ", token.str().c_str(), dom->name.c_str(), line+1);
					dom->printInheritance();
				}
			}
			mode = 0;
			cbk->OnCloseTag(this);
			domBack();
		}else if(!isWhitespace(c)){
			token << c;
		}
	}else if(mode==10){
		if(c=='?' || c=='-'){
			mode = 11;
		}
	}else if(mode==11){
		if(c=='>'){
			mode = 0;
			domBack();
		}else if(c!='?' && c!='-'){
			mode = 10;
		}
	}else if(mode==12){
		if(c=='-'){//first - (comment)
			mode = 13;
		}else if(c=='['){//![CDATA[sanctionText]]
			mode = 14;
			token.reset(); token << c;
		}else{
			mode = 2;
			token.reset(); token << '!' << c;
		}
	}else if(mode==13){
		if(c=='-'){//second - beim (comment)
			mode = 10;
		}else{
			mode = 2;
			token.reset(); token << '!' << '-' << c;
		}
	}else if(mode==14){
		if(c=='['){//second [ of <![CDATA[text]]>
			mode = 15;
		}else if(!(c>='A' && c<='Z')){//error
			mode = 2;
		}//else assert CDATA
		token << c;
	}else if(mode==15){//read CDATA content
		if(c==']'){
			mode = 16;
		}else{
			dom->parent->intermediate << c;
		}
	}else if(mode==16){
		if(c==']'){//second ] from CDATA
			mode = 17;
		}else if(!(c==' ' || c=='\t')){
			dom->parent->intermediate << ']' << c;
			mode = 15;
		}
	}else if(mode==17){
		if(c=='>'){
			mode = 0;
			domBack();
		}else if(!(c==' ' || c=='\t' || c==']')){//in case of legal duplicate ] at end
			dom->parent->intermediate << "]]" << c;
			mode = 15;
		}
	}
}

void XMLParser::domBack(){
	if(dom){
		XMLTag* t = dom->parent;
		delete dom;
		dom = t;
	}else{
		printf("domBack without dom\n");
	}
}

XMLTag* XMLParser::getCurrentDOM(){
	return dom;
}

void XMLParser::OnFinishFile(){
	cbk->OnFinishFile(this);
}

void XMLParser::OnFinishAll(){
	cbk->OnFinishAll(this);
}

void parseFile(XMLParser* parser, std::string file){
	std::ifstream fp(file.c_str(), std::ifstream::binary | std::ifstream::in);
	char c; fp.read(&c, 1);
	while(fp.good()){
		parser->parse(c);
		fp.read(&c, 1);
	}
	parser->OnFinishFile();
}
