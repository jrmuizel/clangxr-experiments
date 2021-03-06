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
    } else if (tokenKind == CXToken_Literal) {
      return "literal";
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
				CXCursor null_cursor = clang_getNullCursor();
				CXSourceLocation loc = clang_getCursorLocation(cur_cursor);
				unsigned inst_line;
				unsigned spelling_line;
				clang_getInstantiationLocation(loc, NULL, &inst_line, NULL, NULL);
				clang_getSpellingLocation(loc, NULL, &spelling_line, NULL, NULL);
				printf("<span class=\"%s c%d_%s t_%s l_i%d_s%d %s\">",
                                       cursor_kind_to_string(cur_kind, clang_getTokenKind(cur_token)),
                                       cur_kind,
                                       clang_getCString(clang_getCursorKindSpelling(cur_kind)),
                                       token_kind_to_string(clang_getTokenKind(cur_token)),
				       inst_line, spelling_line,
				       clang_equalCursors(null_cursor, cur_cursor) ? "nullcur" : "");
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

unsigned ast_depth = 0;

void print_padding()
{
  for (unsigned i = 0; i < ast_depth; ++i) {
    printf("  ");
  }
}

void print_cursor_info(CXCursor cursor, bool child)
{
  CXCursorKind kind = clang_getCursorKind(cursor);
  CXSourceRange range = clang_getCursorExtent(cursor);
  CXSourceLocation start = clang_getRangeStart(range),
                   end = clang_getRangeEnd(range);
  CXFile startFile, endFile;
  unsigned startLine, endLine;
  unsigned startColumn, endColumn;
  unsigned startOffset, endOffset;
  clang_getSpellingLocation(start, &startFile, &startLine, &startColumn, &startOffset);
  clang_getSpellingLocation(end, &endFile, &endLine, &endColumn, &endOffset);
  printf("\"%s\" (%s:%d) [%s:%d(%d)..%s:%d(%d)]",
         // cursor spelling
         clang_getCString(clang_getCursorSpelling(cursor)),
         // cursor kind spelling
         clang_getCString(clang_getCursorKindSpelling(kind)),
         // cursor kind
         kind,
         // extents:
           // start file
           clang_getCString(clang_getFileName(startFile)),
           // start line
           startLine,
           // start column
           startColumn,
           // end file
           clang_getCString(clang_getFileName(endFile)),
           // end line
           endLine,
           // end column
           endColumn
      );

  // print the contents of the range
  if (!child || !startFile) {
    return;
  }
  FILE * file = fopen(clang_getCString(clang_getFileName(startFile)), "r");
  fseek(file, startOffset, SEEK_SET);
  char *text = (char*)malloc(endOffset - startOffset + 1);
  char *origtext = text;
  text[endOffset - startOffset] = '\0';
  fread(text, 1, endOffset - startOffset, file);
  printf("<a href=\"#\" onclick=\"this.className='on'; return false;\">+<span>");
  while (text && *text) {
    char buf[2] = {'\0', '\0'};
    switch (*text) {
      case '\n':
        printf("\\n");
        break;
      case '\r':
        printf("\\r");
        break;
      case '\t':
        printf("\\t");
        break;
      case '<':
        printf("&lt;");
        break;
      case '>':
        printf("&gt;");
        break;
      case '&':
        printf("&amp;");
        break;
      case '"':
        printf("&quot;");
        break;
      default:
        buf[0] = *text;
        printf("%s", buf);
        break;
    }
    ++text;
  }
  fclose(file);
  free(origtext);
  printf("</span></a> ");
}

CXChildVisitResult cursor_visitor(CXCursor cursor, CXCursor parent, CXClientData)
{
  // print the padding
  print_padding();
  print_cursor_info(cursor, true);
  printf(" ");
  print_cursor_info(parent, false);
  printf("\n");
  ++ast_depth;
  clang_visitChildren(cursor, cursor_visitor, NULL);
  --ast_depth;
  return CXChildVisit_Continue;
}

void print_ast(CXTranslationUnit TU)
{
  CXCursor tuCursor = clang_getTranslationUnitCursor(TU);
  printf("<style> a span { display: none } a.on {visibility: hidden} a.on span { display: inline; color: green; visibility: visible; cursor: default; text-decoration: none } </style>");
  printf("<h1>AST</h1><pre>");
  clang_visitChildren(tuCursor, cursor_visitor, NULL);
  printf("</pre>");
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

        print_ast(TU);

	clang_disposeString(filename);
	clang_disposeTranslationUnit(TU);
	clang_disposeIndex(Index);
	return 0;
}

/* vim: set noexpandtab */
