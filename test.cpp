#include <stdlib.h>
#include <clang-c/Index.h>

//#define printf (void)

struct source_position {
	int line_no;
	int column_no;
};


//XXX: this can probably be replaced by clang_getLocationForOffset
struct source_position
get_endLocation(const char* file)
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

void crash() {
    *((int*)4) = 4;
}

const char *cursor_kind_to_string(enum CXCursorKind kind, enum CXTokenKind tokenKind)
{
  if (tokenKind == CXToken_Comment) {
    return "comment";
  }
  if (clang_isDeclaration(kind)) {
    return "declaration";
  } else if (clang_isReference(kind)) {
    return "reference";
  } else if (clang_isExpression(kind)) {
    return "operator";
  } else if (clang_isStatement(kind)) {
    if (tokenKind == CXToken_Punctuation) {
      return "operator";
    } else if (tokenKind == CXToken_Keyword) {
      return "keyword";
    } else if (tokenKind == CXToken_Identifier) {
      return "statement"; // XXX ehsan: we should figure out what we're dealing with here.
                          // hint: |case foo:|
    }
    printf("tokenkind: %d\n", tokenKind);
    puts("statement?");
    crash();
  } else if (clang_isInvalid(kind)) {
    if (tokenKind == CXToken_Punctuation) {
      return "punctuation";
    } else if (tokenKind == CXToken_Keyword) {
      return "keyword";
    } else if (tokenKind == CXToken_Identifier) {
      return "macro?"; // XXX ehsan: I got here on NULL
    }
    printf("tokenkind: %d\n", tokenKind);
    puts("invalid?");
    crash();
  } else if (clang_isTranslationUnit(kind)) {
    puts("TU?");
    crash();
  } else if (clang_isPreprocessing(kind)) {
    return "pp";
  } else if (clang_isUnexposed(kind)) {
    puts("unexposed?");
    crash();
  }
}

const char *token_kind_to_string(CXTokenKind kind)
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
void syntax_hilight(CXTranslationUnit TU, CXSourceRange full_range, const char *file)
{
	CXToken *tokens;
	CXCursor *cursors;
	unsigned num_tokens;
	clang_tokenize(TU, full_range, &tokens, &num_tokens);
	cursors = new CXCursor[num_tokens];
	clang_annotateTokens(TU, tokens, num_tokens, cursors);
	FILE *f = fopen(file, "r");
	char buf[4096];
	size_t bytes_read;
	int token_i = 0;
	CXToken cur_token;
	CXCursor cur_cursor;
	cur_cursor = cursors[token_i];
	cur_token = tokens[token_i++];
	enum CXCursorKind cur_kind = clang_getCursorKind(cur_cursor);
	CXSourceRange cur_token_range = clang_getTokenExtent(TU, cur_token);
	CXSourceLocation cur_token_start = clang_getRangeStart(cur_token_range);
	CXSourceLocation cur_token_end = clang_getRangeEnd(cur_token_range);
	unsigned start_offset;
	unsigned end_offset;
	clang_getInstantiationLocation(cur_token_start, NULL, NULL, NULL, &start_offset);
	clang_getInstantiationLocation(cur_token_end, NULL, NULL, NULL, &end_offset);
	int offset = 0;
	printf("<style>\n"                               \
	       ".punctuation { color: #646464 }\n"       \
	       ".literal { color: #2a00ff }\n"           \
	       ".reference { color: #aa7700 }\n"         \
	       ".comment { color: #3f5fbf }\n"           \
	       ".keyword { color: #7f0055 }\n"           \
	       ".constants { color: #0066cc }\n"         \
	       ".declaration { color: #ff1493 }\n"       \
               ".pp { color: #100500 }\n"                \
	       "</style>");
	printf("<pre>");
	do {
		bytes_read = fread(buf, 1, sizeof(buf), f);
		int i;
		for (i=0; i<bytes_read; i++) {
			if (offset == start_offset) {
			  if(cur_kind == 70) {
			    CXSourceLocation location = clang_getCursorLocation(cur_cursor);
			    CXFile file;
			    clang_getSpellingLocation(location, &file, 0, 0, 0);
			    printf("%s", clang_getCString(clang_getFileName(file)));
			    printf("T%sT", clang_getCString(clang_getTokenSpelling(TU, cur_token)));
			  }
			  //printf("%s", clang_getCString(clang_getCursorSpelling(cur_cursor)));
				printf("<span class=\"%s\">", cursor_kind_to_string(cur_kind, clang_getTokenKind(cur_token)));
			}
			printf("%c", buf[i]);
			offset++;
			if (offset == end_offset) {
				printf("</span>");
				if (token_i < num_tokens) {
					cur_cursor = cursors[token_i];
					cur_token = tokens[token_i++];
					cur_kind = clang_getCursorKind(cur_cursor);
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

	CXString filename = clang_getTranslationUnitSpelling(TU);
	CXFile file = clang_getFile(TU, clang_getCString(filename));

	CXSourceLocation begin = clang_getLocation(TU, file, 1, 1);
	struct source_position end_pos = get_endLocation(clang_getCString(filename));
	CXSourceLocation end = clang_getLocation(TU, file, end_pos.line_no, end_pos.column_no);
	CXSourceRange full_range = clang_getRange(begin, end);

	syntax_hilight(TU, full_range, clang_getCString(filename));

	clang_disposeString(filename);
	clang_disposeTranslationUnit(TU);
	clang_disposeIndex(Index);
	return 0;
}

/* vim: set noexpandtab */
