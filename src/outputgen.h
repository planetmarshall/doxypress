/************************************************************************
*
* Copyright (C) 2014-2020 Barbara Geller & Ansel Sermersheim
* Copyright (C) 1997-2014 by Dimitri van Heesch
*
* DoxyPress is free software: you can redistribute it and/or
* modify it under the terms of the GNU General Public License version 2
* as published by the Free Software Foundation.
*
* DoxyPress is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
*
* Documents produced by DoxyPress are derivative works derived from the
* input used in their production; they are not affected by this license.
*
*************************************************************************/

#ifndef OUTPUTGEN_H
#define OUTPUTGEN_H

#include <QFile>
#include <QStack>
#include <QTextStream>

#include <index.h>
#include <section.h>

class ClassDiagram;
class DotClassGraph;
class DotInclDepGraph;
class DotCallGraph;
class DotDirDeps;
class DotGfxHierarchyTable;
class DotGroupCollaboration;
class DocNode;
class MemberDef;
class GroupDef;
class Definition;

struct DocLinkInfo {
   QString name;
   QString ref;
   QString url;
   QString anchor;
};

struct SourceLinkInfo {
   QString file;
   int line;
   QString ref;
   QString url;
   QString anchor;
};

/** Output interface for code parser.
 */
class CodeOutputInterface
{
 public:
   virtual ~CodeOutputInterface() {}

   /*! Writes an code fragment to the output. This function should keep
    *  spaces visible, should break lines at a newline and should convert
    *  tabs to the right number of spaces.
    */
   virtual void codify(const QString &s) = 0 ;

   /*! Writes a link to an object in a code fragment.
    *  \param ref      If this is non-zero, the object is to be found in
    *                  an external documentation file.
    *  \param file     The file in which the object is located.
    *  \param anchor   The anchor uniquely identifying the object within
    *                  the file.
    *  \param name     The text to display as a placeholder for the link.
    *  \param tooltip  The tooltip to display when the mouse is on the link.
    */
   virtual void writeCodeLink(const QString &ref, const QString &file, const QString &anchor,
                              const QString &name, const QString &tooltip) = 0;

   /*! Writes the line number of a source listing
    *  \param ref        External reference (when imported from a tag file)
    *  \param file       The file part of the URL pointing to the docs.
    *  \param anchor     The anchor part of the URL pointing to the docs.
    *  \param lineNumber The line number to write
    */
   virtual void writeLineNumber(const QString &ref, const QString &file, const QString &anchor, int lineNumber) = 0;

   /*! Writes a tool tip definition
    *  \param id       unique identifier for the tooltip
    *  \param docInfo  Info about the symbol's documentation.
    *  \param decl     full declaration of the symbol (for functions)
    *  \param desc     brief description for the symbol
    *  \param defInfo  Info about the symbol's definition in the source code
    *  \param declInfo Info about the symbol's declaration in the source code
    */
   virtual void writeTooltip(const QString &id, const DocLinkInfo &docInfo, const QString &decl, const QString &desc,
                             const SourceLinkInfo &defInfo, const SourceLinkInfo &declInfo ) = 0;

   virtual void startCodeLine(bool hasLineNumbers) = 0;

   /*! Ends a line of code started with startCodeLine() */
   virtual void endCodeLine() = 0;

   /*! Starts a block with a certain meaning. Used for syntax highlighting,
    *  which elements of the same type are rendered using the same 'font class'.
    *  \param clsName The category name.
    */
   virtual void startFontClass(const QString &clsName) = 0;

   /*! Ends a block started with startFontClass() */
   virtual void endFontClass() = 0;

   /*! Write an anchor to a source listing. */
   virtual void writeCodeAnchor(const QString &name) = 0;

   virtual void setCurrentDoc(QSharedPointer<Definition> context, const QString &anchor, bool isSourceFile) = 0;
   virtual void addWord(const QString &word, bool hiPriority) = 0;
};

/** Base Interface used for generating output outside of the
 *  comment blocks.
 *
 *  This abstract class is used by output generation functions
 *  to generate the output for a specific format,
 *  or a list of formats (see OutputList). This interface
 *  contains functions that generate fragments of the output.
 */
class BaseOutputDocInterface : public CodeOutputInterface
{
 public:
   virtual ~BaseOutputDocInterface() {}

   enum ParamListTypes { Param, RetVal, Exception };
   enum SectionTypes { EnumValues,  Examples};

   virtual bool parseText(const QString &s)  {
      return s.isEmpty();
   }

   /*! Start of a bullet list: e.g. \c \<ul\> in html. startItemListItem() is
    *  Used for the bullet items.
    */
   virtual void startItemList()  = 0;

   /*! Writes a list item for a bullet or enumerated
    *  list: e.g. \c \<li\> in html
    */
   virtual void startItemListItem()  = 0;

   /*! Writes a list item for a bullet or enumerated
    *  list: e.g. \c \</li\> in html
    */
   virtual void endItemListItem()  = 0;

   /*! Ends a bullet list: e.g. \c \</ul\> in html */
   virtual void endItemList()    = 0;

   /*! Writes an ASCII string to the output. Converts characters that have
    *  A special meaning, like \c & in html.
    */
   virtual void docify(const QString &text) = 0;

   /*! Writes a single ASCII character to the output. Converts characters
    *  that have a special meaning.
    */
   virtual void writeChar(char c) = 0;

   /*! Writes an ASCII string to the output, \e without converting
    *  special characters.
    */
   virtual void writeString(const QString &text) = 0;

   /*! Starts a new paragraph */
   //virtual void newParagraph()   = 0;

   /*! Starts a new paragraph */
   virtual void startParagraph(const QString &className) = 0;

   /*! Ends a paragraph */
   virtual void endParagraph() = 0;

   /*! Writes a link to an object in the documentation.
    *  \param ref    If this is non-zero, the object is to be found in
    *                an external documentation file.
    *  \param file   The file in which the object is located.
    *  \param anchor The anchor uniquely identifying the object within
    *                the file.
    *  \param name   The text to display as a placeholder for the link.
    */
   virtual void writeObjectLink(const QString &ref, const QString &file, const QString &anchor, const QString &name) = 0;

   /*! Starts a (link to an) URL found in the documentation.
    *  \param url    The URL to link to.
    */
   virtual void startHtmlLink(const QString &url) = 0;

   /*! Ends a link started by startHtmlLink().
    */
   virtual void endHtmlLink() = 0;


   /*! Changes the text font to bold face. The bold section ends with endBold() */
   virtual void startBold()      = 0;

   /*! End a section of text displayed in bold face. */
   virtual void endBold()        = 0;

   /*! Changes the text font to fixed size. The section ends with endTypewriter() */
   virtual void startTypewriter() = 0;

   /*! End a section of text displayed in typewriter style. */
   virtual void endTypewriter() = 0;

   /*! Changes the text font to italic. The italic section ends with
    *  endEmphasis()
    */
   virtual void startEmphasis() = 0;

   /*! Ends a section of text displayed in italic. */
   virtual void endEmphasis() = 0;

   /*! Starts a source code fragment. The fragment will be
    *  fed to the code parser (see code_cstyle..h) for syntax highlighting
    *  and cross-referencing. The fragment ends by a call to
    *  endCodeFragment()
    */
   virtual void startCodeFragment() = 0;

   /*! Ends a source code fragment
    */
   virtual void endCodeFragment() = 0;


   /*! Writes a horizontal ruler to the output */
   virtual void writeRuler() = 0;

   /*! Starts a description list: e.g. \c \<dl\> in HTML
    *  Items are surrounded by startDescItem() and endDescItem()
    */
   virtual void startDescription() = 0;

   /*! Ends a description list: e.g. \c \</dl\> in HTML */
   virtual void endDescription() = 0;

   /*! Starts an item of a description list: e.g. \c \<dt\> in HTML. */
   virtual void startDescItem() = 0;

   virtual void startDescForItem() = 0;
   virtual void endDescForItem() = 0;

   /*! Ends an item of a description list and starts the
    *  description itself: e.g. \c \</dt\> in HTML.
    */
   virtual void endDescItem() = 0;

   virtual void startCenter() = 0;
   virtual void endCenter() = 0;
   virtual void startSmall() = 0;
   virtual void endSmall() = 0;

   virtual void startSimpleSect(SectionTypes t, const QString &file, const QString &anchor, const QString &title) = 0;

   virtual void endSimpleSect() = 0;
   virtual void startParamList(ParamListTypes t, const QString &title) = 0;
   virtual void endParamList() = 0;

   //virtual void writeDescItem() = 0;
   virtual void startTitle() = 0;
   virtual void endTitle()   = 0;

   virtual void writeAnchor(const QString &fileName, const QString &name) = 0;
   virtual void startSection(const QString &, const QString &, SectionInfo::SectionType) = 0;
   virtual void endSection(const QString &, SectionInfo::SectionType) = 0;

   virtual void lineBreak(const QString &style) = 0;
   virtual void addIndexItemName(const QString &s1, const QString &s2) = 0;

   virtual void writeNonBreakableSpace(int) = 0;
   virtual void startEnumTable() = 0;
   virtual void endEnumTable() = 0;

   virtual void startDescTable(const QString &title) = 0;
   virtual void endDescTable() = 0;
   virtual void startDescTableRow() = 0;
   virtual void endDescTableRow() = 0;
   virtual void startDescTableTitle() = 0;
   virtual void endDescTableTitle() = 0;
   virtual void startDescTableData() = 0;
   virtual void endDescTableData() = 0;

   virtual void startTextLink(const QString &file, const QString &anchor) = 0;
   virtual void endTextLink() = 0;
   virtual void startPageRef() = 0;
   virtual void endPageRef(const QString &, const QString &) = 0;
   virtual void startSubsection() = 0;
   virtual void endSubsection() = 0;
   virtual void startSubsubsection() = 0;
   virtual void endSubsubsection() = 0;
};

/** Abstract output generator.
 *
 *  Subclass this class to add support for a new output format
 */
class OutputGenerator : public BaseOutputDocInterface
{
 public:
   enum OutputType { Html, Latex, Man, RTF, XML, DEF, Perl };

   OutputGenerator();
   virtual ~OutputGenerator();

   // generic generator methods
   virtual void enable() = 0;
   virtual void disable() = 0;
   virtual void enableIf(OutputType o) = 0;
   virtual void disableIf(OutputType o) = 0;
   virtual void disableIfNot(OutputType o) = 0;
   virtual bool isEnabled(OutputType o) = 0;
   virtual OutputGenerator *get(OutputType o) = 0;

   void startPlainFile(const QString &name);
   void endPlainFile();

   bool isEnabled() const {
      return active;
   }

   void pushGeneratorState();
   void popGeneratorState();

   virtual void writeDoc(DocNode *, QSharedPointer<Definition> ctx, QSharedPointer<MemberDef> md) = 0;

   // structural output interface
   virtual void startFile(const QString &name, const QString &manName, const QString &title) = 0;
   virtual void writeSearchInfo() = 0;
   virtual void writeFooter(const QString &navPath) = 0;
   virtual void endFile() = 0;
   virtual void startIndexSection(IndexSections) = 0;
   virtual void endIndexSection(IndexSections) = 0;
   virtual void writePageLink(const QString &, bool) = 0;
   virtual void startProjectNumber() = 0;
   virtual void endProjectNumber() = 0;
   virtual void writeStyleInfo(int part) = 0;
   virtual void startTitleHead(const QString &) = 0;
   virtual void endTitleHead(const QString &fileName, const QString &name) = 0;
   virtual void startIndexListItem() = 0;
   virtual void endIndexListItem()   = 0;
   virtual void startIndexList() = 0;
   virtual void endIndexList()   = 0;
   virtual void startIndexKey() = 0;
   virtual void endIndexKey()   = 0;

   virtual void startIndexValue(bool) = 0;
   virtual void endIndexValue(const QString &, bool) = 0;

   virtual void startIndexItem(const QString &ref, const QString &file) = 0;
   virtual void endIndexItem(const QString &ref, const QString &file) = 0;

   virtual void startGroupHeader(int) = 0;
   virtual void endGroupHeader(int) = 0;
   virtual void startMemberSections() = 0;
   virtual void endMemberSections() = 0;
   virtual void startHeaderSection() = 0;
   virtual void endHeaderSection() = 0;
   virtual void startMemberHeader(const QString &anchor) = 0;
   virtual void endMemberHeader() = 0;
   virtual void startMemberSubtitle() = 0;
   virtual void endMemberSubtitle() = 0;
   virtual void startMemberDocList() = 0;
   virtual void endMemberDocList() = 0;
   virtual void startMemberList() = 0;
   virtual void endMemberList() = 0;
   virtual void startInlineHeader() = 0;
   virtual void endInlineHeader() = 0;
   virtual void startAnonTypeScope(int) = 0;
   virtual void endAnonTypeScope(int) = 0;
   virtual void startMemberItem(const QString &, int, const QString &, bool deprecated = false) = 0;
   virtual void endMemberItem() = 0;
   virtual void startMemberTemplateParams() = 0;
   virtual void endMemberTemplateParams(const QString &, const QString &) = 0;
   virtual void startMemberGroupHeader(bool) = 0;
   virtual void endMemberGroupHeader() = 0;
   virtual void startMemberGroupDocs() = 0;
   virtual void endMemberGroupDocs() = 0;
   virtual void startMemberGroup() = 0;
   virtual void endMemberGroup(bool) = 0;
   virtual void insertMemberAlign(bool) = 0;

   virtual void startMemberDoc(const QString &, const QString &, const QString &, const QString &, bool) = 0;
   virtual void endMemberDoc(bool) = 0;

   virtual void startDoxyAnchor(const QString &fName, const QString &manName,
                                const QString &anchor, const QString &name, const QString &args) = 0;

   virtual void endDoxyAnchor(const QString &fileName, const QString &anchor) = 0;
   virtual void writeLatexSpacing() = 0;

   virtual void writeStartAnnoItem(const QString &type, const QString &file, const QString &path, const QString &name) = 0;

   virtual void writeEndAnnoItem(const QString &name) = 0;
   virtual void startMemberDescription(const QString &anchor, const QString &inheritId) = 0;
   virtual void endMemberDescription() = 0;
   virtual void startMemberDeclaration() = 0;

   virtual void endMemberDeclaration(const QString &anchor, const QString &inheritId) = 0;
   virtual void writeInheritedSectionTitle(const QString &id, const QString &ref, const QString &file, const QString &anchor,
                                           const QString &title, const QString &name) = 0;
   virtual void startIndent() = 0;
   virtual void endIndent() = 0;
   virtual void writeSynopsis() = 0;
   virtual void startClassDiagram() = 0;
   virtual void endClassDiagram(const ClassDiagram &, const QString &, const QString &) = 0;
   virtual void startDotGraph() = 0;
   virtual void endDotGraph(const DotClassGraph &g) = 0;
   virtual void startInclDepGraph() = 0;
   virtual void endInclDepGraph(const DotInclDepGraph &g) = 0;
   virtual void startGroupCollaboration() = 0;
   virtual void endGroupCollaboration(const DotGroupCollaboration &g) = 0;
   virtual void startCallGraph() = 0;
   virtual void endCallGraph(const DotCallGraph &g) = 0;
   virtual void startDirDepGraph() = 0;
   virtual void endDirDepGraph(const DotDirDeps &g) = 0;
   virtual void writeGraphicalHierarchy(const DotGfxHierarchyTable &g) = 0;
   virtual void startQuickIndices() = 0;
   virtual void endQuickIndices() = 0;
   virtual void writeSplitBar(const QString &) = 0;
   virtual void writeNavigationPath(const QString &) = 0;
   virtual void writeLogo() = 0;
   virtual void writeQuickLinks(bool compact, HighlightedItem hli, const QString &file) = 0;
   virtual void writeSummaryLink(const QString &file, const QString &anchor, const QString &title, bool first) = 0;
   virtual void startContents() = 0;
   virtual void endContents() = 0;

   virtual void startPageDoc(const QString &pageTitle) {
      (void) pageTitle;
   };

   virtual void endPageDoc() {};
   virtual void startTextBlock(bool) = 0;
   virtual void endTextBlock(bool) = 0;
   virtual void lastIndexPage() = 0;
   virtual void startMemberDocPrefixItem() = 0;
   virtual void endMemberDocPrefixItem() = 0;
   virtual void startMemberDocName(bool) = 0;
   virtual void endMemberDocName() = 0;
   virtual void startParameterType(bool, const QString &key) = 0;
   virtual void endParameterType() = 0;
   virtual void startParameterName(bool) = 0;
   virtual void endParameterName(bool, bool, bool) = 0;
   virtual void startParameterList(bool) = 0;
   virtual void endParameterList() = 0;
   virtual void exceptionEntry(const QString &, bool) = 0;

   virtual void startConstraintList(const QString &) = 0;
   virtual void startConstraintParam() = 0;
   virtual void endConstraintParam() = 0;
   virtual void startConstraintType() = 0;
   virtual void endConstraintType() = 0;
   virtual void startConstraintDocs() = 0;
   virtual void endConstraintDocs() = 0;
   virtual void endConstraintList() = 0;

   virtual void startMemberDocSimple(bool isEnum) = 0;
   virtual void endMemberDocSimple(bool isEnum) = 0;
   virtual void startInlineMemberType() = 0;
   virtual void endInlineMemberType() = 0;
   virtual void startInlineMemberName() = 0;
   virtual void endInlineMemberName() = 0;
   virtual void startInlineMemberDoc() = 0;
   virtual void endInlineMemberDoc() = 0;

   virtual void startLabels() = 0;
   virtual void writeLabel(const QString &, bool) = 0;
   virtual void endLabels() = 0;

 protected:
   QTextStream m_textStream;

   QFile    m_file;
   QString  m_fileName;
   QString  m_dir;
   bool     active;

   QStack<bool> genStack;

 private:
   OutputGenerator(const OutputGenerator &o);
   OutputGenerator &operator=(const OutputGenerator &o);
};

/** Interface used for generating documentation.
 *
 *  This abstract class is used by several functions
 *  to generate the output for a specific format.
 *  This interface contains some state saving and changing
 *  functions for dealing with format specific output.
 */
class OutputDocInterface : public BaseOutputDocInterface
{
 public:
   virtual ~OutputDocInterface() {}

   /*! Create a new output generator. This can later by appended
    *  to the current one using append().
    */
   //virtual OutputDocInterface *clone() = 0;

   /*! Disables all output formats except format \a o
    *  (useful for OutputList only)
    */
   virtual void disableAllBut(OutputGenerator::OutputType o) = 0;

   /*! Enables all output formats as far as they have been enabled in
    *  the project file. (useful for OutputList only)
    */
   virtual void enableAll() = 0;

   /*! Disables all output formats (useful for OutputList only) */
   virtual void disableAll() = 0;

   /*! Disables a specific output format (useful for OutputList only) */
   virtual void disable(OutputGenerator::OutputType o) = 0;

   /*! Enables a specific output format (useful for OutputList only) */
   virtual void enable(OutputGenerator::OutputType o) = 0;

   /*! Check whether a specific output format is currently enabled
    *  (useful for OutputList only)
    */
   virtual bool isEnabled(OutputGenerator::OutputType o) = 0;

   /*! Appends the output generated by generator \a g to this
    *  generator.
    */
   //virtual void append(const OutputDocInterface *g) = 0;

   /*! Pushes the state of the current generator (or list of
    *  generators) on a stack.
    */
   virtual void pushGeneratorState() = 0;

   /*! Pops the state of the current generator (or list of
    *  generators) on a stack. Should be preceded by a call
    *  the pushGeneratorState().
    */
   virtual void popGeneratorState() = 0;
};


#endif
