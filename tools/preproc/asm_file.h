// Copyright(c) 2016 YamaArashi
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in
// all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
// THE SOFTWARE.

#ifndef ASM_FILE_H
#define ASM_FILE_H

#include <cstdarg>
#include <cstdint>
#include <string>
#include <set>
#include "preproc.h"

enum class Directive
{
    Include,
    String,
    Braille,
    Unknown
};

enum class LabelType
{
    Nonlocal,
    Local,
    Global,
    NotALabel
};

class LocalLabelInfo;

class AsmFile
{
public:
    AsmFile(std::string filename);
    AsmFile(AsmFile&& other);
    AsmFile(const AsmFile&) = delete;
    ~AsmFile();
    Directive GetDirective();
    LabelType TryParseLabel(std::string& returnLabel);
    void FindAndReplaceLocalLabels(std::string& outputLine);
    std::string ReadPath();
    int ReadString(unsigned char* s);
    int ReadBraille(unsigned char* s);
    bool IsAtEnd();
    void OutputLine();
    void OutputLocation();
    void FindUndefinedLocalLabelsThenClear();

private:
    char* m_buffer;
    long m_pos;
    long m_size;
    long m_lineNum;
    long m_lineStart;
    std::string m_filename;
    std::string m_curNonLocalLabel;
    bool m_nonLocalLabelFound;
    std::map<std::string, LocalLabelInfo> m_localLabels;
    bool m_identified = false;
    //struct 
    //std::vector<

    bool ConsumeComma();
    int ReadPadLength();
    void RemoveComments();
    bool CheckForDirective(std::string name);
    void SkipWhitespace();
    void ExpectEmptyRestOfLine();
    void ReportDiagnostic(const char* type, const char* format, std::va_list args);
    void ReportDiagnosticWhere(const char* type, long lineNum, const char* format, std::va_list args);
    void RaiseError(const char * format, ...);
    void RaiseErrorWhereButContinue(long lineNum, const char * format, ...);
    void RaiseErrorButContinue(const char * format, ...);
    void RaiseWarning(const char* format, ...);
    void TryPrintErrorHeaderMessage();
};

class LocalLabelInfo
{
    public:
        LocalLabelInfo();
        LocalLabelInfo(long lineNum);
        //~LocalLabelInfo();

        bool defined;
        std::set<long> lineNums;
};

#endif // ASM_FILE_H
