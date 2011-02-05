#include <clang-c/Index.h>

struct source_position {
	int line_no;
	int column_no;
};


//XXX: this can probably be replaced by clang_getLocationForOffset
struct source_position
get_endLocation(char* file)
{
	FILE *f = fopen(file, "r");
	char buf[4096];
	size_t bytes_read;
	int line_no = 1;
	int column_no = 1;
	do {
		bytes_read = fread(buf, 1, sizeof(buf), f);
		int i;
		for (i=0; i<bytes_read; i++) {
			if (buf[i] == '\n') {
				line_no++;
				column_no = 1;
			} else {
				column_no++;
			}
		}
	} while (bytes_read > 0);
	fclose(f);
	struct source_position ret;
	ret.line_no = line_no;
	ret.column_no = column_no;
	return ret;

}

char *token_kind_to_string(CXTokenKind kind)
{
	switch (kind) {
		case CXToken_Punctuation:
			return "punctuation";
		case CXToken_Keyword:
			return "keyword";
		case CXToken_Identifier:
			return "identifier";
		case CXToken_Literal:
			return "literal";
		case CXToken_Comment:
			return "comment";
	}
	return "unknown";
}
void syntax_hilight(CXTranslationUnit TU, CXSourceRange full_range, char *file)
{
	CXToken *tokens;
	unsigned num_tokens;
	clang_tokenize(TU, full_range, &tokens, &num_tokens);
	FILE *f = fopen(file, "r");
	char buf[4096];
	size_t bytes_read;
	int token_i = 0;
	CXToken cur_token;
	cur_token = tokens[token_i++];
	CXSourceRange cur_token_range = clang_getTokenExtent(TU, cur_token);
	CXSourceLocation cur_token_start = clang_getRangeStart(cur_token_range);
	CXSourceLocation cur_token_end = clang_getRangeEnd(cur_token_range);
	unsigned start_offset;
	unsigned end_offset;
	clang_getInstantiationLocation(cur_token_start, NULL, NULL, NULL, &start_offset);
	clang_getInstantiationLocation(cur_token_end, NULL, NULL, NULL, &end_offset);
	int offset = 0;
	printf("<style>.punctuation { color: #646464 } .literal { color: #2a00ff } .identifier { color: #aa7700 } .comment { color: #3f5fbf } .keyword { color: #7f0055 }</style>");
	printf("<pre>");
	do {
		bytes_read = fread(buf, 1, sizeof(buf), f);
		int i;
		for (i=0; i<bytes_read; i++) {
			if (offset == start_offset) {
				printf("<span class=\"%s\">", token_kind_to_string(clang_getTokenKind(cur_token)));
			}
			printf("%c", buf[i]);
			offset++;
			if (offset == end_offset) {
				printf("</span>");
				if (token_i < num_tokens) {
					cur_token = tokens[token_i++];
					CXSourceRange cur_token_range = clang_getTokenExtent(TU, cur_token);
					CXSourceLocation cur_token_start = clang_getRangeStart(cur_token_range);
					CXSourceLocation cur_token_end = clang_getRangeEnd(cur_token_range);
					clang_getInstantiationLocation(cur_token_start, NULL, NULL, NULL, &start_offset);
					clang_getInstantiationLocation(cur_token_end, NULL, NULL, NULL, &end_offset);
				}
			}
			/*
			   if (buf[i] == '\n') {
			   line_no++;
			   column_no = ;
			   } else {
			   column_no++;
			   }*/
		}
	} while (bytes_read > 0);
	printf("</pre>");
	fclose(f);

}

void print_CXString(CXString str)
{
	printf("%s\n", clang_getCString(str));
}

int main(int argc, const char *const *argv)
{
	CXIndex Index = clang_createIndex(0, 0);
	CXTranslationUnit TU = clang_parseTranslationUnit(Index, 0,
		argv, argc, 0, 0, CXTranslationUnit_None);

	CXFile file = clang_getFile(TU, "test.c");
#if 0
	// this doesn't work because the root cursor doesn't seem to be 
	CXCursor root_cursor = clang_getTranslationUnitCursor(TU);
	//CXFile file = clang_getFile(TU, "test.c");
	CXFile file;
	CXSourceLocation root_location = clang_getCursorLocation(root_cursor);
	if (clang_equalLocations(root_location, clang_getNullLocation())) {
		printf("bad location");
	}
	clang_getInstantiationLocation(root_location, &file, NULL, NULL, NULL);
	print_CXString(clang_getFileName(file));
#endif
	CXSourceLocation begin = clang_getLocation(TU, file, 1, 1);
	struct source_position end_pos = get_endLocation("test.c");
	CXSourceLocation end = clang_getLocation(TU, file, end_pos.line_no, end_pos.column_no);
	CXSourceRange full_range = clang_getRange(begin, end);
	CXToken *tokens;
	unsigned num_tokens;
	clang_tokenize(TU, full_range, &tokens, &num_tokens);
#if 0
	printf("num tokens: %d\n", num_tokens);
	for (int i=0; i<num_tokens; i++) {
		CXString spelling = clang_getTokenSpelling(TU, tokens[i]);
		printf("%d: %s\n", clang_getTokenKind(tokens[i]), clang_getCString(spelling));
		clang_disposeString(spelling);
	}
#endif
	clang_disposeTokens(TU, tokens, num_tokens);
	syntax_hilight(TU, full_range, "test.c");
	clang_disposeTranslationUnit(TU);
	clang_disposeIndex(Index);
	return 0;
}
