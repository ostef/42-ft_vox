#include "Core.hpp"
#include "Renderer.hpp"

static const char *Log_Shader_Preprocessor = "Shader Preprocessor";

static bool IsAtEnd(Lexer *lexer)
{
    return lexer->cursor >= lexer->file_contents.length;
}

static void Advance(Lexer *lexer, s64 count = 1)
{
    s64 i = 0;
    while (!IsAtEnd(lexer) && i < count)
    {
        lexer->cursor += 1;
        i += 1;
    }
}

static bool EqualsString(Lexer *lexer, String str)
{
    if (lexer->cursor + str.length > lexer->file_contents.length)
        return false;

    String remaining;
    remaining.data = lexer->file_contents.data + lexer->cursor;
    remaining.length = str.length;

    return remaining == str;
}

static bool MatchString(Lexer *lexer, String str)
{
    if (EqualsString(lexer, str))
    {
        Advance(lexer, str.length);
        return true;
    }

    return false;
}

static void SkipComments(Lexer *lexer)
{
    while (!IsAtEnd(lexer))
    {
        if (MatchString(lexer, "//"))
        {
            while (!IsAtEnd(lexer))
            {
                if (lexer->file_contents[lexer->cursor] == '\n')
                {
                    Advance(lexer);
                    break;
                }

                Advance(lexer);
            }
        }
        else if (MatchString(lexer, "/*"))
        {
            while (!IsAtEnd(lexer) && !MatchString(lexer, "*/"))
                Advance(lexer);
        }
        else
        {
            break;
        }
    }
}

static bool EqualsAlphanum(Lexer *lexer, String str)
{
    if (!EqualsString(lexer, str))
        return false;

    return IsAtEnd(lexer) || !isalnum(lexer->file_contents[lexer->cursor]);
}

static bool MatchAlphanum(Lexer *lexer, String str)
{
    if (EqualsAlphanum(lexer, str))
    {
        Advance(lexer, str.length);
        return true;
    }

    return false;
}

static Result<String> ParseStringLiteral(Lexer *lexer)
{
    if (!MatchString(lexer, "\""))
        return Result<String>::Bad(false);

    Array<char> result = {};
    result.allocator = heap;

    while (!IsAtEnd(lexer))
    {
        // @Todo: escape sequences
        if (lexer->file_contents[lexer->cursor] == '"')
            break;
        if (lexer->file_contents[lexer->cursor] == '\n')
            break;

        ArrayPush(&result, lexer->file_contents[lexer->cursor]);
        Advance(lexer);
    }

    if (IsAtEnd(lexer) || lexer->file_contents[lexer->cursor] != '"')
    {
        LogError(Log_Shader_Preprocessor, "Unclosed string literal");
        return Result<String>::Bad(false);
    }

    Advance(lexer);

    return Result<String>::Good(String{result.count, result.data}, true);
}

static bool AddFile(ShaderPreprocessor *pp, String filename)
{
    String base_directory = "";
    if (pp->current_lexer)
    {
        base_directory = GetParentDirectory(pp->current_lexer->filename);
        filename = JoinStrings(base_directory, filename, "/", temp);
    }

    // @Todo:
    // filename = GetAbsoluteFilename(filename);

    foreach (i, pp->file_stack)
    {
        if (pp->file_stack[i].filename == filename)
        {
            LogError(Log_Shader_Preprocessor, "Circular include of file '%.*s'", filename.length, filename.data);
            return false;
        }
    }

    auto contents = ReadEntireFile(filename);
    if (!contents.ok)
    {
        LogError(Log_Shader_Preprocessor, "Could not read file '%.*s'", filename.length, filename.data);
        return false;
    }

    ArrayPush(&pp->all_loaded_files, CloneString(filename, heap));

    auto lexer = ArrayPush(&pp->file_stack);
    lexer->filename = CloneString(filename, heap);
    lexer->file_contents = contents.value;
    lexer->cursor = 0;
    pp->current_lexer = lexer;

    return true;
}

bool InitShaderPreprocessor(ShaderPreprocessor *pp, String filename)
{
    pp->all_loaded_files.allocator = heap;
    pp->file_stack.allocator = heap;
    pp->string_builder.allocator = heap;

    return AddFile(pp, filename);
}

void DestroyShaderPreprocessor(ShaderPreprocessor *pp)
{
    foreach (i, pp->all_loaded_files)
        Free(pp->all_loaded_files[i].data, heap);

    foreach (i, pp->file_stack)
    {
        Free(pp->file_stack[i].filename.data, heap);
        Free(pp->file_stack[i].file_contents.data, heap);
    }

    ArrayFree(&pp->all_loaded_files);
    ArrayFree(&pp->file_stack);
    ArrayFree(&pp->string_builder);
    *pp = {};
}

static bool IsAtEnd(ShaderPreprocessor *pp)
{
    return pp->file_stack.count == 0;
}

ShaderPreprocessResult PreprocessShader(ShaderPreprocessor *pp)
{
    while (!IsAtEnd(pp))
    {
        s64 cursor_start = pp->current_lexer->cursor;
        SkipComments(pp->current_lexer);
        s64 cursor_end = pp->current_lexer->cursor;

        // Write comments to output
        for (s64 i = cursor_start; i < cursor_end; i += 1)
        {
            ArrayPush(&pp->string_builder, pp->current_lexer->file_contents[i]);
        }

        if (IsAtEnd(pp->current_lexer))
        {
            Free(pp->current_lexer->filename.data, heap);
            Free(pp->current_lexer->file_contents.data, heap);

            ArrayPop(&pp->file_stack);

            if (pp->file_stack.count > 0)
                pp->current_lexer = &pp->file_stack[pp->file_stack.count - 1];

            continue;
        }

        if (MatchAlphanum(pp->current_lexer, "#include"))
        {
            while (!IsAtEnd(pp->current_lexer) && isspace(pp->current_lexer->file_contents[pp->current_lexer->cursor]))
                Advance(pp->current_lexer);

            auto str_result = ParseStringLiteral(pp->current_lexer);
            if (!str_result.ok)
                return {};

            String filename = str_result.value;
            bool ok = AddFile(pp, filename);
            if (!ok)
                return {};
        }
        else
        {
            ArrayPush(&pp->string_builder, pp->current_lexer->file_contents[pp->current_lexer->cursor]);
            Advance(pp->current_lexer);
        }
    }

    ShaderPreprocessResult result{};
    result.all_loaded_files = AllocSlice<String>(pp->all_loaded_files.count, heap);
    foreach (i, pp->all_loaded_files)
        result.all_loaded_files[i] = CloneString(pp->all_loaded_files[i], heap);

    result.source_code = CloneString(String{pp->string_builder.count, pp->string_builder.data}, heap);

    result.ok = true;

    return result;
}
