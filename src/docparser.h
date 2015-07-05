/*************************************************************************
 *
 * Copyright (C) 2014-2015 Barbara Geller & Ansel Sermersheim 
 * Copyright (C) 1997-2014 by Dimitri van Heesch.
 * All rights reserved.    
 *
 * Permission to use, copy, modify, and distribute this software and its
 * documentation under the terms of the GNU General Public License version 2
 * is hereby granted. No representations are made about the suitability of
 * this software for any purpose. It is provided "as is" without express or
 * implied warranty. See the GNU General Public License for more details.
 *
 * Documents produced by DoxyPress are derivative works derived from the
 * input used in their production; they are not affected by this license.
 *
*************************************************************************/

#ifndef DOCPARSER_H
#define DOCPARSER_H

#include <QList>

#include <stdio.h>

#include <definition.h>
#include <docvisitor.h>
#include <htmlattrib.h>
#include <memberdef.h>
#include <membergroup.h>
#include <section.h>

/*! Main entry point for the documentation parser.
 *  @param fileName  File in which the documentation block is found (or the
 *                   name of the example file in case isExample is true).
 *  @param startLine Line at which the documentation block is found.
 *  @param context   Class or namespace to which this block belongs.
 *  @param md        Member definition to which the documentation belongs.
 *                   Can be 0.
 *  @param input     String representation of the documentation block.
 *  @param indexWords Indicates whether or not words should be put in the
 *                   search index.
 *  @param isExample true if the documentation belongs to an example.
 *  @param exampleName Base name of the example file (0 if isExample is false).
 *  @param singleLine Output should be presented on a single line, so without
 *                   starting a new paragraph at the end.
 *  @param linkFromIndex true if the documentation is generated from an
 *                   index page. In this case context is not used to determine
 *                   the relative path when making a link.
 *  @returns         Root node of the abstract syntax tree. Ownership of the
 *                   pointer is handed over to the caller.
 */
DocRoot *validatingParseDoc(const QString &fileName, int startLine, QSharedPointer<Definition> context, 
                  QSharedPointer<MemberDef> md, const QString &input, bool indexWords, bool isExample, 
                  const QString &exampleName = QString(), bool singleLine = false, bool linkFromIndex = false);

/*! Main entry point for parsing simple text fragments. These
 *  fragments are limited to words, whitespace and symbols.
 */
DocText *validatingParseText(const char *input);

/*! Searches for section and anchor commands in the input */
void docFindSections(const QString &input, QSharedPointer<Definition> d, MemberGroup *m, const QString &fileName);

/** Abstract node interface with type information. */
class DocNode
{
 public:
   /*! Available node types. */
   enum Kind { Kind_Root           = 0,
               Kind_Word           = 1,
               Kind_WhiteSpace     = 2,
               Kind_Para           = 3,
               Kind_AutoList       = 4,
               Kind_AutoListItem   = 5,
               Kind_Symbol         = 6,
               Kind_URL            = 7,
               Kind_StyleChange    = 8,
               Kind_SimpleSect     = 9,
               Kind_Title          = 10,
               Kind_SimpleList     = 11,
               Kind_SimpleListItem = 12,
               Kind_Section        = 13,
               Kind_Verbatim       = 14,
               Kind_XRefItem       = 15,
               Kind_HtmlList       = 16,
               Kind_HtmlListItem   = 17,
               Kind_HtmlDescList   = 18,
               Kind_HtmlDescData   = 19,
               Kind_HtmlDescTitle  = 20,
               Kind_HtmlTable      = 21,
               Kind_HtmlRow        = 22,
               Kind_HtmlCell       = 23,
               Kind_HtmlCaption    = 24,
               Kind_LineBreak      = 25,
               Kind_HorRuler       = 26,
               Kind_Anchor         = 27,
               Kind_IndexEntry     = 28,
               Kind_Internal       = 29,
               Kind_HRef           = 30,
               Kind_Include        = 31,
               Kind_IncOperator    = 32,
               Kind_HtmlHeader     = 33,
               Kind_Image          = 34,
               Kind_DotFile        = 35,
               Kind_Link           = 36,
               Kind_Ref            = 37,
               Kind_Formula        = 38,
               Kind_SecRefItem     = 39,
               Kind_SecRefList     = 40,
               Kind_SimpleSectSep  = 41,
               Kind_LinkedWord     = 42,
               Kind_ParamSect      = 43,
               Kind_ParamList      = 44,
               Kind_InternalRef    = 45,
               Kind_Copy           = 46,
               Kind_Text           = 47,
               Kind_MscFile        = 48,
               Kind_HtmlBlockQuote = 49,
            
               Kind_ParBlock       = 51,
               Kind_DiaFile        = 52
             };

   /*! Creates a new node */
   DocNode() : m_parent(0), m_insidePre(false) {}

   /*! Destroys a node. */
   virtual ~DocNode() {}

   /*! Returns the kind of node. Provides runtime type information */
   virtual Kind kind() const = 0;

   /*! Returns the parent of this node or 0 for the root node. */
   DocNode *parent() const {
      return m_parent;
   }

   /*! Sets a new parent for this node. */
   void setParent(DocNode *parent) {
      m_parent = parent;
   }

   /*! Acceptor function for node visitors. Part of the visitor pattern.
    *  @param v Abstract visitor.
    */
   virtual void accept(DocVisitor *v) = 0;

   /*! Returns true if this node is inside a preformatted section */
   bool isPreformatted() const {
      return m_insidePre;
   }

 protected:
   /*! Sets whether or not this item is inside a preformatted section */
   void setInsidePreformatted(bool p) {
      m_insidePre = p;
   }

   DocNode *m_parent;

 private:
   bool m_insidePre;
};

/** Default accept implementation for compound nodes in the abstract syntax tree.
 */
template<class T> class CompAccept
{
 public:
   CompAccept() {  }

   virtual ~CompAccept() {
      for (auto item : m_children) {
         delete item;
      }
   }

   void accept(T *obj, DocVisitor *v) {
      v->visitPre(obj);

      for (auto n : m_children) {
         n->accept(v);
      }

      v->visitPost(obj);
   }

   const QList<DocNode *> &children() const {
      return m_children;
   }

   QList<DocNode *> &children() {
      return m_children;
   }

 protected:
   QList<DocNode *> m_children;
};


/** Node representing a word
 */
class DocWord : public DocNode
{
 public:
   DocWord(DocNode *parent, const QString &word);

   QString word() const {
      return m_word;
   }

   Kind kind() const {
      return Kind_Word;
   }

   void accept(DocVisitor *v) {
      v->visit(this);
   }

 private:
   QString m_word;
};

/** Node representing a word that can be linked to something
 */
class DocLinkedWord : public DocNode
{
 public:
   DocLinkedWord(DocNode *parent, const QString &word, const QString &ref, const QString &file,
                 const QString &anchor, const QString &tooltip);

   QString word() const       {
      return m_word;
   }

   Kind kind() const          {
      return Kind_LinkedWord;
   }

   QString file() const       {
      return m_file;
   }
   QString relPath() const    {
      return m_relPath;
   }
   QString ref() const        {
      return m_ref;
   }
   QString anchor() const     {
      return m_anchor;
   }
   QString tooltip() const    {
      return m_tooltip;
   }

   void accept(DocVisitor *v) {
      v->visit(this);
   }

 private:
   QString  m_word;
   QString  m_ref;
   QString  m_file;
   QString  m_relPath;
   QString  m_anchor;
   QString  m_tooltip;
};

/** Node representing an URL (or email address) */
class DocURL : public DocNode
{
 public:
   DocURL(DocNode *parent, const QString &url, bool isEmail) :
      m_url(url), m_isEmail(isEmail) {
      m_parent = parent;
   }

   QString url() const {
      return m_url;
   }

   Kind kind() const {
      return Kind_URL;
   }

   void accept(DocVisitor *v) {
      v->visit(this);
   }

   bool isEmail() const       {
      return m_isEmail;
   }

 private:
   QString m_url;
   bool m_isEmail;
};

/** Node representing a line break */
class DocLineBreak : public DocNode
{
 public:
   DocLineBreak(DocNode *parent) {
      m_parent = parent;
   }
   Kind kind() const          {
      return Kind_LineBreak;
   }
   void accept(DocVisitor *v) {
      v->visit(this);
   }

 private:
};

/** Node representing a horizonal ruler */
class DocHorRuler : public DocNode
{
 public:
   DocHorRuler(DocNode *parent) {
      m_parent = parent;
   }

   Kind kind() const          {
      return Kind_HorRuler;
   }

   void accept(DocVisitor *v) {
      v->visit(this);
   }

 private:
};

/** Node representing an anchor */
class DocAnchor : public DocNode
{
 public:
   DocAnchor(DocNode *parent, const QString &id, bool newAnchor);
   void parse();

   Kind kind() const {
      return Kind_Anchor;
   }

   QString anchor() const {
      return m_anchor;
   }

   QString file() const {
      return m_file;
   }

   void accept(DocVisitor *v) {
      v->visit(this);
   }

 private:
   QString m_anchor;
   QString m_file;
};

/** Node representing a citation of some bibliographic reference */
class DocCite : public DocNode
{
 public:
   DocCite(DocNode *parent, const QString &target, const QString &context);
   Kind kind() const            {
      return Kind_Ref;
   }
   QString file() const        {
      return m_file;
   }
   QString relPath() const     {
      return m_relPath;
   }
   QString ref() const         {
      return m_ref;
   }
   QString anchor() const      {
      return m_anchor;
   }
   QString text() const        {
      return m_text;
   }
   void accept(DocVisitor *v) {
      v->visit(this);
   }

 private:
   QString   m_file;
   QString   m_relPath;
   QString   m_ref;
   QString   m_anchor;
   QString   m_text;
};


/** Node representing a style change */
class DocStyleChange : public DocNode
{
 public:
   enum Style { Bold, Italic, Code, Center, Small, Subscript, Superscript,
                Preformatted, Span, Div };

   DocStyleChange(DocNode *parent, uint position, Style s, bool enable, const HtmlAttribList *attribs = 0)    
      : m_position(position), m_style(s), m_enable(enable) 
   {
      m_parent = parent;

      if (attribs) {
         m_attribs = *attribs;
      }
   }

   DocStyleChange()         
   {
   }

   Kind kind() const {
      return Kind_StyleChange;
   }

   Style style() const {
      return m_style;
   }

   const char *styleString() const;

   bool enable() const {
      return m_enable;
   }

   uint position() const  {
      return m_position;
   }

   void accept(DocVisitor *v) {
      v->visit(this);
   }
   const HtmlAttribList &attribs() const {
      return m_attribs;
   }

 private:
   uint     m_position;
   Style    m_style;
   bool     m_enable;
   HtmlAttribList m_attribs;
};

/** Node representing a special symbol */
class DocSymbol : public DocNode
{
 public:
   enum SymType { Sym_Unknown = -1,
                  Sym_nbsp, Sym_iexcl, Sym_cent, Sym_pound, Sym_curren,
                  Sym_yen, Sym_brvbar, Sym_sect, Sym_uml, Sym_copy,
                  Sym_ordf, Sym_laquo, Sym_not, Sym_shy, Sym_reg,
                  Sym_macr, Sym_deg, Sym_plusmn, Sym_sup2, Sym_sup3,
                  Sym_acute, Sym_micro, Sym_para, Sym_middot, Sym_cedil,
                  Sym_sup1, Sym_ordm, Sym_raquo, Sym_frac14, Sym_frac12,
                  Sym_frac34, Sym_iquest, Sym_Agrave, Sym_Aacute, Sym_Acirc,
                  Sym_Atilde, Sym_Auml, Sym_Aring, Sym_AElig, Sym_Ccedil,
                  Sym_Egrave, Sym_Eacute, Sym_Ecirc, Sym_Euml, Sym_Igrave,
                  Sym_Iacute, Sym_Icirc, Sym_Iuml, Sym_ETH, Sym_Ntilde,
                  Sym_Ograve, Sym_Oacute, Sym_Ocirc, Sym_Otilde, Sym_Ouml,
                  Sym_times, Sym_Oslash, Sym_Ugrave, Sym_Uacute, Sym_Ucirc,
                  Sym_Uuml, Sym_Yacute, Sym_THORN, Sym_szlig, Sym_agrave,
                  Sym_aacute, Sym_acirc, Sym_atilde, Sym_auml, Sym_aring,
                  Sym_aelig, Sym_ccedil, Sym_egrave, Sym_eacute, Sym_ecirc,
                  Sym_euml, Sym_igrave, Sym_iacute, Sym_icirc, Sym_iuml,
                  Sym_eth, Sym_ntilde, Sym_ograve, Sym_oacute, Sym_ocirc,
                  Sym_otilde, Sym_ouml, Sym_divide, Sym_oslash, Sym_ugrave,
                  Sym_uacute, Sym_ucirc, Sym_uuml, Sym_yacute, Sym_thorn,
                  Sym_yuml, Sym_fnof, Sym_Alpha, Sym_Beta, Sym_Gamma,
                  Sym_Delta, Sym_Epsilon, Sym_Zeta, Sym_Eta, Sym_Theta,
                  Sym_Iota, Sym_Kappa, Sym_Lambda, Sym_Mu, Sym_Nu,
                  Sym_Xi, Sym_Omicron, Sym_Pi, Sym_Rho, Sym_Sigma,
                  Sym_Tau, Sym_Upsilon, Sym_Phi, Sym_Chi, Sym_Psi,
                  Sym_Omega, Sym_alpha, Sym_beta, Sym_gamma, Sym_delta,
                  Sym_epsilon, Sym_zeta, Sym_eta, Sym_theta, Sym_iota,
                  Sym_kappa, Sym_lambda, Sym_mu, Sym_nu, Sym_xi,
                  Sym_omicron, Sym_pi, Sym_rho, Sym_sigmaf, Sym_sigma,
                  Sym_tau, Sym_upsilon, Sym_phi, Sym_chi, Sym_psi,
                  Sym_omega, Sym_thetasym, Sym_upsih, Sym_piv, Sym_bull,
                  Sym_hellip, Sym_prime, Sym_Prime, Sym_oline, Sym_frasl,
                  Sym_weierp, Sym_image, Sym_real, Sym_trade, Sym_alefsym,
                  Sym_larr, Sym_uarr, Sym_rarr, Sym_darr, Sym_harr,
                  Sym_crarr, Sym_lArr, Sym_uArr, Sym_rArr, Sym_dArr,
                  Sym_hArr, Sym_forall, Sym_part, Sym_exist, Sym_empty,
                  Sym_nabla, Sym_isin, Sym_notin, Sym_ni, Sym_prod,
                  Sym_sum, Sym_minus, Sym_lowast, Sym_radic, Sym_prop,
                  Sym_infin, Sym_ang, Sym_and, Sym_or, Sym_cap,
                  Sym_cup, Sym_int, Sym_there4, Sym_sim, Sym_cong,
                  Sym_asymp, Sym_ne, Sym_equiv, Sym_le, Sym_ge,
                  Sym_sub, Sym_sup, Sym_nsub, Sym_sube, Sym_supe,
                  Sym_oplus, Sym_otimes, Sym_perp, Sym_sdot, Sym_lceil,
                  Sym_rceil, Sym_lfloor, Sym_rfloor, Sym_lang, Sym_rang,
                  Sym_loz, Sym_spades, Sym_clubs, Sym_hearts, Sym_diams,
                  Sym_quot, Sym_amp, Sym_lt, Sym_gt, Sym_OElig,
                  Sym_oelig, Sym_Scaron, Sym_scaron, Sym_Yuml, Sym_circ,
                  Sym_tilde, Sym_ensp, Sym_emsp, Sym_thinsp, Sym_zwnj,
                  Sym_zwj, Sym_lrm, Sym_rlm, Sym_ndash, Sym_mdash,
                  Sym_lsquo, Sym_rsquo, Sym_sbquo, Sym_ldquo, Sym_rdquo,
                  Sym_bdquo, Sym_dagger, Sym_Dagger, Sym_permil, Sym_lsaquo,
                  Sym_rsaquo, Sym_euro,

                  /* DoxyPress extensions */
                  Sym_tm, Sym_apos,

                  /* DoxyPresscommands mapped */
                  Sym_BSlash, Sym_At, Sym_Less, Sym_Greater, Sym_Amp,
                  Sym_Dollar, Sym_Hash, Sym_DoubleColon, Sym_Percent, Sym_Pipe,
                  Sym_Quot, Sym_Minus
                };

   enum PerlType { Perl_unknown = 0, Perl_string, Perl_char, Perl_symbol, Perl_umlaut,
                   Perl_acute, Perl_grave, Perl_circ, Perl_slash, Perl_tilde,
                   Perl_cedilla, Perl_ring
                 };

   typedef struct PerlSymb {
      const char     *symb;
      const PerlType  type;
   } PerlSymb;

   DocSymbol(DocNode *parent, SymType s) :
      m_symbol(s) {
      m_parent = parent;
   }
   SymType symbol() const     {
      return m_symbol;
   }
   Kind kind() const          {
      return Kind_Symbol;
   }
   void accept(DocVisitor *v) {
      v->visit(this);
   }
   static SymType decodeSymbol(const QString &symName);

 private:
   SymType  m_symbol;
};

/** Node representing some amount of white space */
class DocWhiteSpace : public DocNode
{
 public:
   DocWhiteSpace(DocNode *parent, const QString &chars) 
      : m_chars(chars) {
      m_parent = parent;
   }

   Kind kind() const          {
      return Kind_WhiteSpace;
   }

   QString chars() const      {
      return m_chars;
   }

   void accept(DocVisitor *v) {
      v->visit(this);
   }

 private:
   QString  m_chars;
};

/** Node representing a verbatim, unparsed text fragment */
class DocVerbatim : public DocNode
{
 public:
   enum Type { Code, HtmlOnly, ManOnly, LatexOnly, RtfOnly, XmlOnly, Verbatim, Dot, Msc, DocbookOnly, PlantUML};

   DocVerbatim(DocNode *parent, const QString &context,const QString &text, Type t, bool isExample,
               const QString &exampleFile, bool isBlock = false, const QString &lang = QString());

   Kind kind() const {
      return Kind_Verbatim;
   }

   Type type() const {
      return m_type;
   }

   QString text() const {
      return m_text;
   }

   QString context() const {
      return m_context;
   }

   void accept(DocVisitor *v) {
      v->visit(this);
   }

   bool isExample() const {
      return m_isExample;
   }

   QString exampleFile() const {
      return m_exampleFile;
   }

   QString relPath() const {
      return m_relPath;
   }

   QString language() const {
      return m_lang;
   }

   bool isBlock() const {
      return m_isBlock;
   }

 private:
   QString  m_context;
   QString  m_text;
   Type     m_type;
   bool     m_isExample;
   QString  m_exampleFile;
   QString  m_relPath;
   QString  m_lang;
   bool     m_isBlock;
};


/** Node representing an included text block from file */
class DocInclude : public DocNode
{
 public:
   enum Type { Include, DontInclude, VerbInclude, HtmlInclude, LatexInclude, IncWithLines, Snippet };
   DocInclude(DocNode *parent, const QString &file, const QString context, Type t,
              bool isExample, const QString exampleFile, const QString blockId) 
      : m_file(file), m_context(context), m_type(t), m_isExample(isExample), m_exampleFile(exampleFile), m_blockId(blockId) { 

      m_parent = parent;
   }

   Kind kind() const            {
      return Kind_Include;
   }

   QString file() const        {
      return m_file;
   }

   QString extension() const   {
      int i = m_file.lastIndexOf('.');

      if (i != -1) {
         return m_file.right(m_file.length() - i);
      } else {
         return "";
      }
   }

   Type type() const            {
      return m_type;
   }

   QString text() const        {
      return m_text;
   }

   QString context() const     {
      return m_context;
   }

   QString blockId() const     {
      return m_blockId;
   }

   bool isExample() const       {
      return m_isExample;
   }

   QString exampleFile() const {
      return m_exampleFile;
   }

   void accept(DocVisitor *v)   {
      v->visit(this);
   }

   void parse();

 private:
   QString  m_file;
   QString  m_context;
   QString  m_text;
   Type     m_type;
   bool     m_isExample;
   QString  m_exampleFile;
   QString  m_blockId;
};

/** Node representing a include/dontinclude operator block */
class DocIncOperator : public DocNode
{
 public:
   enum Type { Line, SkipLine, Skip, Until };
   DocIncOperator(DocNode *parent, Type t, const QString &pat, const QString &context, bool isExample, 
                  const QString &exampleFile) 
      : m_type(t), m_pattern(pat), m_context(context), m_isFirst(false), m_isLast(false), 
        m_isExample(isExample), m_exampleFile(exampleFile) {

      m_parent = parent;
   }

   Kind kind() const           {
      return Kind_IncOperator;
   }
   Type type() const           {
      return m_type;
   }
   QString text() const        {
      return m_text;
   }
   QString pattern() const     {
      return m_pattern;
   }
   QString context() const     {
      return m_context;
   }
   void accept(DocVisitor *v)  {
      v->visit(this);
   }
   bool isFirst() const        {
      return m_isFirst;
   }
   bool isLast() const         {
      return m_isLast;
   }
   void markFirst(bool v = true) {
      m_isFirst = v;
   }
   void markLast(bool v = true)  {
      m_isLast = v;
   }
   bool isExample() const      {
      return m_isExample;
   }
   QString exampleFile() const {
      return m_exampleFile;
   }
   void parse();

 private:
   Type     m_type;
   QString  m_text;
   QString  m_pattern;
   QString  m_context;
   bool     m_isFirst;
   bool     m_isLast;
   bool     m_isExample;
   QString  m_exampleFile;
};

/** Node representing an item of a cross-referenced list */
class DocFormula : public DocNode
{
 public:
   DocFormula(DocNode *parent, int id);
   Kind kind() const          {
      return Kind_Formula;
   }
   QString name() const       {
      return m_name;
   }
   QString text() const       {
      return m_text;
   }
   QString relPath() const    {
      return m_relPath;
   }
   int id() const             {
      return m_id;
   }
   void accept(DocVisitor *v) {
      v->visit(this);
   }
   bool isInline()            {
      return m_text.length() > 0 ? m_text.at(0) != '\\' : true;
   }

 private:
   QString  m_name;
   QString  m_text;
   QString  m_relPath;
   int m_id;
};

/** Node representing an entry in the index. */
class DocIndexEntry : public DocNode
{
 public:
   DocIndexEntry(DocNode *parent, QSharedPointer<Definition> scope, QSharedPointer<MemberDef> md)
      : m_scope(scope), m_member(md) {
      m_parent = parent;
   }

   Kind kind() const {
      return Kind_IndexEntry;
   }

   int parse();

   QSharedPointer<Definition> scope() const    {
      return m_scope;
   }

   QSharedPointer<MemberDef> member() const    {
      return m_member;
   }

   QString entry() const        {
      return m_entry;
   }
   void accept(DocVisitor *v)   {
      v->visit(this);
   }

 private:
   QString m_entry;

   QSharedPointer<Definition> m_scope;
   QSharedPointer<MemberDef>  m_member;
};

/** Node representing a copy of documentation block. */
class DocCopy : public DocNode
{
 public:
   DocCopy(DocNode *parent, const QString &link, bool copyBrief, bool copyDetails)
      : m_link(link), m_copyBrief(copyBrief), m_copyDetails(copyDetails) {
      m_parent = parent;
   }

   Kind kind() const          {
      return Kind_Copy;
   }

   QString link() const       {
      return m_link;
   }

   void accept(DocVisitor * /*v*/) {
      /*CompAccept<DocCopy>::accept(this,v);*/
   }

   void parse(QList<DocNode *> &children);

 private:
   QString  m_link;
   bool     m_copyBrief;
   bool     m_copyDetails;
};

/** Node representing an auto List */
class DocAutoList : public CompAccept<DocAutoList>, public DocNode
{
 public:
   DocAutoList(DocNode *parent, int indent, bool isEnumList, int depth);
   Kind kind() const          {
      return Kind_AutoList;
   }
   bool isEnumList() const    {
      return m_isEnumList;
   }
   int  indent() const        {
      return m_indent;
   }
   int depth() const          {
      return m_depth;
   }
   void accept(DocVisitor *v) {
      CompAccept<DocAutoList>::accept(this, v);
   }
   int parse();

 private:
   int      m_indent;
   bool     m_isEnumList;
   int      m_depth;
};

/** Node representing an item of a auto list */
class DocAutoListItem : public CompAccept<DocAutoListItem>, public DocNode
{
 public:
   DocAutoListItem(DocNode *parent, int indent, int num);
   Kind kind() const          {
      return Kind_AutoListItem;
   }
   int itemNumber() const     {
      return m_itemNum;
   }
   void accept(DocVisitor *v) {
      CompAccept<DocAutoListItem>::accept(this, v);
   }
   int parse();

 private:
   int m_indent;
   int m_itemNum;
};



/** Node representing a simple section title */
class DocTitle : public CompAccept<DocTitle>, public DocNode
{
 public:
   DocTitle(DocNode *parent) {
      m_parent = parent;
   }
   void parse();
   void parseFromString(const QString &title);
   Kind kind() const          {
      return Kind_Title;
   }
   void accept(DocVisitor *v) {
      CompAccept<DocTitle>::accept(this, v);
   }

 private:
};

/** Node representing an item of a cross-referenced list */
class DocXRefItem : public CompAccept<DocXRefItem>, public DocNode
{
 public:
   DocXRefItem(DocNode *parent, int id, const QString &key);

   Kind kind() const          {
      return Kind_XRefItem;
   }
   QString file() const       {
      return m_file;
   }
   QString anchor() const     {
      return m_anchor;
   }
   QString title() const      {
      return m_title;
   }
   QString relPath() const    {
      return m_relPath;
   }
   QString key() const        {
      return m_key;
   }
   void accept(DocVisitor *v) {
      CompAccept<DocXRefItem>::accept(this, v);
   }
   bool parse();

 private:
   int      m_id;
   QString  m_key;
   QString  m_file;
   QString  m_anchor;
   QString  m_title;
   QString  m_relPath;
};

/** Node representing an image */
class DocImage : public CompAccept<DocImage>, public DocNode
{
 public:
   enum Type { Html, Latex, Rtf, DocBook };

   DocImage(DocNode *parent, const HtmlAttribList &attribs, const QString &name, Type t, const QString &url = QString());
   Kind kind() const           {
      return Kind_Image;
   }
   Type type() const           {
      return m_type;
   }
   QString name() const       {
      return m_name;
   }
   bool hasCaption() const     {
      return !m_children.isEmpty();
   }
   QString width() const      {
      return m_width;
   }
   QString height() const     {
      return m_height;
   }
   QString relPath() const    {
      return m_relPath;
   }
   QString url() const        {
      return m_url;
   }
   const HtmlAttribList &attribs() const {
      return m_attribs;
   }
   void accept(DocVisitor *v) {
      CompAccept<DocImage>::accept(this, v);
   }
   void parse();

 private:
   HtmlAttribList m_attribs;
   QString  m_name;
   Type     m_type;
   QString  m_width;
   QString  m_height;
   QString  m_relPath;
   QString  m_url;
};

/** Node representing a dot file */
class DocDotFile : public CompAccept<DocDotFile>, public DocNode
{
 public:
   DocDotFile(DocNode *parent, const QString &name, const QString &context);
   void parse();

   Kind kind() const          {
      return Kind_DotFile;
   }
   QString name() const       {
      return m_name;
   }
   QString file() const       {
      return m_file;
   }
   QString relPath() const    {
      return m_relPath;
   }
   bool hasCaption() const    {
      return !m_children.isEmpty();
   }
   QString width() const      {
      return m_width;
   }
   QString height() const     {
      return m_height;
   }
   QString context() const    {
      return m_context;
   }
   void accept(DocVisitor *v) {
      CompAccept<DocDotFile>::accept(this, v);
   }
 private:
   QString  m_name;
   QString  m_file;
   QString  m_relPath;
   QString  m_width;
   QString  m_height;
   QString  m_context;
};

/** Node representing a msc file */
class DocMscFile : public CompAccept<DocMscFile>, public DocNode
{
 public:
   DocMscFile(DocNode *parent, const QString &name, const QString &context);
   void parse();

   Kind kind() const          {
      return Kind_MscFile;
   }
   QString name() const      {
      return m_name;
   }
   QString file() const      {
      return m_file;
   }
   QString relPath() const   {
      return m_relPath;
   }
   bool hasCaption() const    {
      return !m_children.isEmpty();
   }
   QString width() const     {
      return m_width;
   }
   QString height() const    {
      return m_height;
   }
   QString context() const   {
      return m_context;
   }
   void accept(DocVisitor *v) {
      CompAccept<DocMscFile>::accept(this, v);
   }

 private:
   QString  m_name;
   QString  m_file;
   QString  m_relPath;
   QString  m_width;
   QString  m_height;
   QString  m_context;
};

/** Node representing a dia file */
class DocDiaFile : public CompAccept<DocDiaFile>, public DocNode
{
 public:
   DocDiaFile(DocNode *parent, const QString &name, const QString &context);
   void parse();
   Kind kind() const          {
      return Kind_DiaFile;
   }
   QString name() const      {
      return m_name;
   }
   QString file() const      {
      return m_file;
   }
   QString relPath() const   {
      return m_relPath;
   }
   bool hasCaption() const    {
      return !m_children.isEmpty();
   }
   QString width() const     {
      return m_width;
   }
   QString height() const    {
      return m_height;
   }
   QString context() const   {
      return m_context;
   }
   void accept(DocVisitor *v) {
      CompAccept<DocDiaFile>::accept(this, v);
   }
 private:
   QString  m_name;
   QString  m_file;
   QString  m_relPath;
   QString  m_width;
   QString  m_height;
   QString  m_context;
};

/** Node representing a link to some item */
class DocLink : public CompAccept<DocLink>, public DocNode
{
 public:
   DocLink(DocNode *parent, const QString &target);
   QString parse(bool, bool isXmlLink = false);

   Kind kind() const          {
      return Kind_Link;
   }
   QString file() const       {
      return m_file;
   }
   QString relPath() const    {
      return m_relPath;
   }
   QString ref() const        {
      return m_ref;
   }
   QString anchor() const     {
      return m_anchor;
   }
   void accept(DocVisitor *v) {
      CompAccept<DocLink>::accept(this, v);
   }

 private:
   QString  m_file;
   QString  m_relPath;
   QString  m_ref;
   QString  m_anchor;
   QString  m_refText;
};

/** Node representing a reference to some item */
class DocRef : public CompAccept<DocRef>, public DocNode
{
 public:
   DocRef(DocNode *parent, const QString &target, const QString &context);
   void parse();

   Kind kind() const {
      return Kind_Ref;
   }

   QString file() const{
      return m_file;
   }

   QString relPath() const {
      return m_relPath;
   }
   QString ref() const {
      return m_ref;
   }
   QString anchor() const {
      return m_anchor;
   }

   QString targetTitle() const {
      return m_text;
   }

   bool hasLinkText() const {
      return ! m_children.isEmpty();
   }

   bool refToAnchor() const     {
      return m_refToAnchor;
   }

   bool refToSection() const    {
      return m_refToSection;
   }

   bool isSubPage() const       {
      return m_isSubPage;
   }

   void accept(DocVisitor *v)   {
      CompAccept<DocRef>::accept(this, v);
   }

 private:
   bool m_refToSection;
   bool m_refToAnchor;
   bool m_isSubPage;

   QString   m_file;
   QString   m_relPath;
   QString   m_ref;
   QString   m_anchor;
   QString   m_text;
};

/** Node representing an internal reference to some item */
class DocInternalRef : public CompAccept<DocInternalRef>, public DocNode
{
 public:
   DocInternalRef(DocNode *parent, const QString &target);
   void parse();
   Kind kind() const            {
      return Kind_Ref;
   }
   QString file() const         {
      return m_file;
   }
   QString relPath() const      {
      return m_relPath;
   }
   QString anchor() const       {
      return m_anchor;
   }
   void accept(DocVisitor *v)   {
      CompAccept<DocInternalRef>::accept(this, v);
   }

 private:
   QString   m_file;
   QString   m_relPath;
   QString   m_anchor;
};

/** Node representing a Hypertext reference */
class DocHRef : public CompAccept<DocHRef>, public DocNode
{
 public:
   DocHRef(DocNode *parent, const HtmlAttribList &attribs, const QString &url, const QString &relPath) 
      : m_attribs(attribs), m_url(url), m_relPath(relPath) {
      m_parent = parent;
   }

   int parse();
   QString url() const        {
      return m_url;
   }
   QString relPath() const    {
      return m_relPath;
   }
   Kind kind() const           {
      return Kind_HRef;
   }
   void accept(DocVisitor *v)  {
      CompAccept<DocHRef>::accept(this, v);
   }
   const HtmlAttribList &attribs() const {
      return m_attribs;
   }

 private:
   HtmlAttribList m_attribs;
   QString   m_url;
   QString   m_relPath;
};

/** Node Html heading */
class DocHtmlHeader : public CompAccept<DocHtmlHeader>, public DocNode
{
 public:
   DocHtmlHeader(DocNode *parent, const HtmlAttribList &attribs, int level) :
      m_level(level), m_attribs(attribs) {
      m_parent = parent;
   }
   int level() const                     {
      return m_level;
   }
   Kind kind() const                     {
      return Kind_HtmlHeader;
   }
   const HtmlAttribList &attribs() const {
      return m_attribs;
   }
   void accept(DocVisitor *v) {
      CompAccept<DocHtmlHeader>::accept(this, v);
   }
   int parse();

 private:
   int           m_level;
   HtmlAttribList m_attribs;
};

/** Node representing a Html description item */
class DocHtmlDescTitle : public CompAccept<DocHtmlDescTitle>, public DocNode
{
 public:
   DocHtmlDescTitle(DocNode *parent, const HtmlAttribList &attribs) :
      m_attribs(attribs) {
      m_parent = parent;
   }
   Kind kind() const                     {
      return Kind_HtmlDescTitle;
   }
   const HtmlAttribList &attribs() const {
      return m_attribs;
   }
   void accept(DocVisitor *v) {
      CompAccept<DocHtmlDescTitle>::accept(this, v);
   }
   int parse();

 private:
   HtmlAttribList m_attribs;
};

/** Node representing a Html description list */
class DocHtmlDescList : public CompAccept<DocHtmlDescList>, public DocNode
{
 public:
   DocHtmlDescList(DocNode *parent, const HtmlAttribList &attribs) :
      m_attribs(attribs) {
      m_parent = parent;
   }
   Kind kind() const                     {
      return Kind_HtmlDescList;
   }
   const HtmlAttribList &attribs() const {
      return m_attribs;
   }
   void accept(DocVisitor *v) {
      CompAccept<DocHtmlDescList>::accept(this, v);
   }
   int parse();

 private:
   HtmlAttribList m_attribs;
};

/** Node representing a normal section */
class DocSection : public CompAccept<DocSection>, public DocNode
{
 public:
   DocSection(DocNode *parent, int level, const QString &id) 
      : m_level(level), m_id(id) {
      m_parent = parent;
   }
   Kind kind() const          {
      return Kind_Section;
   }
   int level() const          {
      return m_level;
   }
   QString title() const      {
      return m_title;
   }
   QString anchor() const     {
      return m_anchor;
   }
   QString id() const         {
      return m_id;
   }
   QString file() const       {
      return m_file;
   }
   void accept(DocVisitor *v) {
      CompAccept<DocSection>::accept(this, v);
   }
   int parse();

 private:
   int      m_level;
   QString  m_id;
   QString  m_title;
   QString  m_anchor;
   QString  m_file;
};

/** Node representing a reference to a section */
class DocSecRefItem : public CompAccept<DocSecRefItem>, public DocNode
{
 public:
   DocSecRefItem(DocNode *parent, const QString &target) :
      m_target(target) {
      m_parent = parent;
   }
   Kind kind() const          {
      return Kind_SecRefItem;
   }
   QString target() const     {
      return m_target;
   }
   QString file() const       {
      return m_file;
   }
   QString anchor() const     {
      return m_anchor;
   }
   void accept(DocVisitor *v) {
      CompAccept<DocSecRefItem>::accept(this, v);
   }
   void parse();

 private:
   QString  m_target;
   QString  m_file;
   QString  m_anchor;
};

/** Node representing a list of section references */
class DocSecRefList : public CompAccept<DocSecRefList>, public DocNode
{
 public:
   DocSecRefList(DocNode *parent) {
      m_parent = parent;
   }
   void parse();
   Kind kind() const          {
      return Kind_SecRefList;
   }
   void accept(DocVisitor *v) {
      CompAccept<DocSecRefList>::accept(this, v);
   }

 private:
};

/** Node representing an internal section of documentation */
class DocInternal : public CompAccept<DocInternal>, public DocNode
{
 public:
   DocInternal(DocNode *parent) {
      m_parent = parent;
   }
   int parse(int);
   Kind kind() const          {
      return Kind_Internal;
   }
   void accept(DocVisitor *v) {
      CompAccept<DocInternal>::accept(this, v);
   }

 private:
};

/** Node representing an block of paragraphs */
class DocParBlock : public CompAccept<DocParBlock>, public DocNode
{
 public:
   DocParBlock(DocNode *parent) {
      m_parent = parent;
   }
   int parse();
   Kind kind() const          {
      return Kind_ParBlock;
   }
   void accept(DocVisitor *v) {
      CompAccept<DocParBlock>::accept(this, v);
   }

 private:
};


/** Node representing a simple list */
class DocSimpleList : public CompAccept<DocSimpleList>, public DocNode
{
 public:
   DocSimpleList(DocNode *parent) {
      m_parent = parent;
   }
   Kind kind() const          {
      return Kind_SimpleList;
   }
   void accept(DocVisitor *v) {
      CompAccept<DocSimpleList>::accept(this, v);
   }
   int parse();

 private:
};

/** Node representing a Html list */
class DocHtmlList : public CompAccept<DocHtmlList>, public DocNode
{
 public:
   enum Type { Unordered, Ordered };
   DocHtmlList(DocNode *parent, const HtmlAttribList &attribs, Type t) :
      m_type(t), m_attribs(attribs) {
      m_parent = parent;
   }
   Kind kind() const          {
      return Kind_HtmlList;
   }
   Type type() const          {
      return m_type;
   }
   void accept(DocVisitor *v) {
      CompAccept<DocHtmlList>::accept(this, v);
   }
   const HtmlAttribList &attribs() const {
      return m_attribs;
   }
   int parse();
   int parseXml();

 private:
   Type          m_type;
   HtmlAttribList m_attribs;
};

/** Node representing a simple section */
class DocSimpleSect : public CompAccept<DocSimpleSect>, public DocNode
{
 public:
   enum Type {
      Unknown, See, Return, Author, Authors, Version, Since, Date,
      Note, Warning, Copyright, Pre, Post, Invar, Remark, Attention, User, Rcs
   };
   DocSimpleSect(DocNode *parent, Type t);
   virtual ~DocSimpleSect();
   Kind kind() const       {
      return Kind_SimpleSect;
   }
   Type type() const       {
      return m_type;
   }
   QString typeString() const;
   void accept(DocVisitor *v);
   int parse(bool userTitle, bool needsSeparator);
   int parseRcs();
   int parseXml();
   void appendLinkWord(const QString &word);

 private:
   Type            m_type;
   DocTitle       *m_title;
};

/** Node representing a separator between two simple sections of the
 *  same type.
 */
class DocSimpleSectSep : public DocNode
{
 public:
   DocSimpleSectSep(DocNode *parent) {
      m_parent = parent;
   }
   Kind kind() const {
      return Kind_SimpleSectSep;
   }
   void accept(DocVisitor *v) {
      v->visit(this);
   }

 private:
};

/** Node representing a parameter section */
class DocParamSect : public CompAccept<DocParamSect>, public DocNode
{
   friend class DocParamList;

 public:
   enum Type {
      Unknown, Param, RetVal, Exception, TemplateParam
   };
   enum Direction {
      In = 1, Out = 2, InOut = 3, Unspecified = 0
   };
   DocParamSect(DocNode *parent, Type t)
      : m_type(t), m_hasInOutSpecifier(false), m_hasTypeSpecifier(false) {
      m_parent = parent;
   }
   int parse(const QString &cmdName, bool xmlContext, Direction d);
   Kind kind() const          {
      return Kind_ParamSect;
   }
   Type type() const          {
      return m_type;
   }
   void accept(DocVisitor *v) {
      CompAccept<DocParamSect>::accept(this, v);
   }
   bool hasInOutSpecifier() const {
      return m_hasInOutSpecifier;
   }
   bool hasTypeSpecifier() const  {
      return m_hasTypeSpecifier;
   }

 private:
   Type            m_type;
   bool            m_hasInOutSpecifier;
   bool            m_hasTypeSpecifier;
};

/** Node representing a paragraph in the documentation tree */
class DocPara : public CompAccept<DocPara>, public DocNode
{
 public:
   DocPara(DocNode *parent) :
      m_isFirst(false), m_isLast(false) {
      m_parent = parent;
   }

   int parse(bool skipParse = false, int token = 0);

   Kind kind() const {
      return Kind_Para;
   }

   bool isEmpty() const {
      return m_children.isEmpty();
   }

   void accept(DocVisitor *v)  {
      CompAccept<DocPara>::accept(this, v);
   }
   void markFirst(bool v = true) {
      m_isFirst = v;
   }
   void markLast(bool v = true)  {
      m_isLast = v;
   }
   bool isFirst() const        {
      return m_isFirst;
   }
   bool isLast() const         {
      return m_isLast;
   }

   int handleCommand(const QString &cmdName);
   int handleHtmlStartTag(const QString &tagName, const HtmlAttribList &tagHtmlAttribs);
   int handleHtmlEndTag(const QString &tagName);
   int handleSimpleSection(DocSimpleSect::Type t, bool xmlContext = false);
   int handleXRefItem();
   int handleParamSection(const QString &cmdName, DocParamSect::Type t, bool xmlContext, int direction);
   void handleIncludeOperator(const QString &cmdName, DocIncOperator::Type t);
   void handleImage(const QString &cmdName);
   void handleDotFile(const QString &cmdName);
   void handleMscFile(const QString &cmdName);
   void handleDiaFile(const QString &cmdName);
   void handleInclude(const QString &cmdName, DocInclude::Type t);
   void handleLink(const QString &cmdName, bool isJavaLink);
   void handleCite();
   void handleRef(const QString &cmdName);
   void handleSection(const QString &cmdName);
   void handleSortId();
   void handleInheritDoc();
 
   int handleStartCode();
   int handleHtmlHeader(const HtmlAttribList &tagHtmlAttribs, int level);

   bool injectToken(int tok, const QString &tokText);

 private:
   QString  m_sectionId;
   bool m_isFirst;
   bool m_isLast;
};

/** Node representing a parameter list. */
class DocParamList : public DocNode
{
 public:
   DocParamList(DocNode *parent, DocParamSect::Type t, DocParamSect::Direction d)
      : m_type(t), m_dir(d), m_isFirst(true), m_isLast(true) {     
      m_parent = parent;
   }

   virtual ~DocParamList()        
   { }

   Kind kind() const {
      return Kind_ParamList;
   }

   const QList<DocNode *> &parameters()  {
      return m_params;
   }

   const QList<DocNode *> &paramTypes()  {
      return m_paramTypes;
   }

   DocParamSect::Type type() const {
      return m_type;
   }

   DocParamSect::Direction direction() const {
      return m_dir;
   }

   void markFirst(bool b = true)     {
      m_isFirst = b;
   }

   void markLast(bool b = true)      {
      m_isLast = b;
   }

   bool isFirst() const            {
      return m_isFirst;
   }

   bool isLast() const             {
      return m_isLast;
   }

   void accept(DocVisitor *v) {
      v->visitPre(this);

      for (auto n : m_paragraphs)  {
         n.accept(v);
      }

      v->visitPost(this);
   }

   int parse(const QString &cmdName);
   int parseXml(const QString &paramName);

 private:
   QList<DocPara>          m_paragraphs;
   QList<DocNode *>        m_params;
   QList<DocNode *>        m_paramTypes;
   DocParamSect::Type      m_type;
   DocParamSect::Direction m_dir;
   bool                    m_isFirst;
   bool                    m_isLast;
};

/** Node representing a simple list item */
class DocSimpleListItem : public DocNode
{
 public:
   DocSimpleListItem(DocNode *parent) {
      m_paragraph = new DocPara(this);
      m_parent = parent;
   }
   int parse();
   virtual ~DocSimpleListItem() {
      delete m_paragraph;
   }
   Kind kind() const            {
      return Kind_SimpleListItem;
   }
   void accept(DocVisitor *v) {
      v->visitPre(this);
      m_paragraph->accept(v);
      v->visitPost(this);
   }

 private:
   DocPara *m_paragraph;
};

/** Node representing a HTML list item */
class DocHtmlListItem : public CompAccept<DocHtmlListItem>, public DocNode
{
 public:
   DocHtmlListItem(DocNode *parent, const HtmlAttribList &attribs, int num) :
      m_attribs(attribs), m_itemNum(num) {
      m_parent = parent;
   }

   Kind kind() const                     {
      return Kind_HtmlListItem;
   }

   int itemNumber() const                {
      return m_itemNum;
   }

   const HtmlAttribList &attribs() const {
      return m_attribs;
   }

   void accept(DocVisitor *v) {
      CompAccept<DocHtmlListItem>::accept(this, v);
   }

   int parse();
   int parseXml();

 private:
   HtmlAttribList m_attribs;
   int            m_itemNum;
};

/** Node representing a HTML description data */
class DocHtmlDescData : public CompAccept<DocHtmlDescData>, public DocNode
{
 public:
   DocHtmlDescData(DocNode *parent) {
      m_parent = parent;
   }
   Kind kind() const                     {
      return Kind_HtmlDescData;
   }
   const HtmlAttribList &attribs() const {
      return m_attribs;
   }
   void accept(DocVisitor *v) {
      CompAccept<DocHtmlDescData>::accept(this, v);
   }
   int parse();

 private:
   HtmlAttribList m_attribs;
};

/** Node representing a HTML table cell */
class DocHtmlCell : public CompAccept<DocHtmlCell>, public DocNode
{
   friend class DocHtmlTable;
 public:
   enum Alignment { Left, Right, Center };
   DocHtmlCell(DocNode *parent, const HtmlAttribList &attribs, bool isHeading) :
      m_isHeading(isHeading),
      m_isFirst(false), m_isLast(false), m_attribs(attribs),
      m_rowIdx(-1), m_colIdx(-1) {
      m_parent = parent;
   }
   bool isHeading() const      {
      return m_isHeading;
   }
   bool isFirst() const        {
      return m_isFirst;
   }
   bool isLast() const         {
      return m_isLast;
   }
   Kind kind() const           {
      return Kind_HtmlCell;
   }
   void accept(DocVisitor *v)  {
      CompAccept<DocHtmlCell>::accept(this, v);
   }
   void markFirst(bool v = true) {
      m_isFirst = v;
   }
   void markLast(bool v = true)  {
      m_isLast = v;
   }
   const HtmlAttribList &attribs() const {
      return m_attribs;
   }
   int parse();
   int parseXml();
   int rowIndex() const        {
      return m_rowIdx;
   }
   int columnIndex() const     {
      return m_colIdx;
   }
   int rowSpan() const;
   int colSpan() const;
   Alignment alignment() const;

 private:
   void setRowIndex(int idx)    {
      m_rowIdx = idx;
   }
   void setColumnIndex(int idx) {
      m_colIdx = idx;
   }
   bool           m_isHeading;
   bool           m_isFirst;
   bool           m_isLast;
   HtmlAttribList m_attribs;
   int            m_rowIdx;
   int            m_colIdx;
};

/** Node representing a HTML table caption */
class DocHtmlCaption : public CompAccept<DocHtmlCaption>, public DocNode
{
 public:
   DocHtmlCaption(DocNode *parent, const HtmlAttribList &attribs) :
      m_attribs(attribs) {
      m_parent = parent;
   }
   Kind kind() const          {
      return Kind_HtmlCaption;
   }
   void accept(DocVisitor *v) {
      CompAccept<DocHtmlCaption>::accept(this, v);
   }
   const HtmlAttribList &attribs() const {
      return m_attribs;
   }
   int parse();

 private:
   HtmlAttribList m_attribs;
};

/** Node representing a HTML table row */
class DocHtmlRow : public CompAccept<DocHtmlRow>, public DocNode
{
   friend class DocHtmlTable;

 public:
   DocHtmlRow(DocNode *parent, const HtmlAttribList &attribs) :
      m_attribs(attribs), m_visibleCells(-1), m_rowIdx(-1) {
      m_parent = parent;
   }

   Kind kind() const          {
      return Kind_HtmlRow;
   }

   uint numCells() const      {
      return m_children.count();
   }

   void accept(DocVisitor *v) {
      CompAccept<DocHtmlRow>::accept(this, v);
   }

   const HtmlAttribList &attribs() const {
      return m_attribs;
   }

   int parse();
   int parseXml(bool header);

   bool isHeading() const     {

      if ( m_children.isEmpty() ) {
         return false;
      }

      DocNode * obj1 = m_children.first();
      DocHtmlCell *obj2 = dynamic_cast<DocHtmlCell *>(obj1);

      if (! obj2) {
         return false;
      }

      return obj2->isHeading();
   }

   void setVisibleCells(int n) {
      m_visibleCells = n;
   }

   int visibleCells() const    {
      return m_visibleCells;
   }

   int rowIndex() const        {
      return m_rowIdx;
   }

 private:
   void setRowIndex(int idx)    {
      m_rowIdx = idx;
   }
   HtmlAttribList m_attribs;
   int m_visibleCells;
   int m_rowIdx;
};

/** Node representing a HTML table */
class DocHtmlTable : public CompAccept<DocHtmlTable>, public DocNode
{
 public:
   DocHtmlTable(DocNode *parent, const HtmlAttribList &attribs)
      : m_attribs(attribs) {
      m_caption = 0;
      m_parent = parent;
   }
   ~DocHtmlTable()         {
      delete m_caption;
   }
   Kind kind() const       {
      return Kind_HtmlTable;
   }
   uint numRows() const    {
      return m_children.count();
   }
   bool hasCaption()       {
      return m_caption != 0;
   }
   const HtmlAttribList &attribs() const {
      return m_attribs;
   }
   int parse();
   int parseXml();
   uint numColumns() const {
      return m_numCols;
   }
   void accept(DocVisitor *v);

 private:
   void computeTableGrid();
   DocHtmlCaption    *m_caption;
   HtmlAttribList     m_attribs;
   int m_numCols;
};

/** Node representing an HTML blockquote */
class DocHtmlBlockQuote : public CompAccept<DocHtmlBlockQuote>, public DocNode
{
 public:
   DocHtmlBlockQuote(DocNode *parent, const HtmlAttribList &attribs)
      : m_attribs(attribs) {
      m_parent = parent;
   }
   Kind kind() const       {
      return Kind_HtmlBlockQuote;
   }
   int parse();
   void accept(DocVisitor *v) {
      CompAccept<DocHtmlBlockQuote>::accept(this, v);
   }
   const HtmlAttribList &attribs() const {
      return m_attribs;
   }

 private:
   HtmlAttribList m_attribs;
};

/** Root node of a text fragment */
class DocText : public CompAccept<DocText>, public DocNode
{
 public:
   DocText() {}

   Kind kind() const       {
      return Kind_Text;
   }

   void accept(DocVisitor *v) {
      CompAccept<DocText>::accept(this, v);
   }

   void parse();

   bool isEmpty() const    {
      return m_children.isEmpty();
   }
};

/** Root node of documentation tree */
class DocRoot : public CompAccept<DocRoot>, public DocNode
{
 public:
   DocRoot(bool indent, bool sl) : m_indent(indent), m_singleLine(sl) {}

   Kind kind() const       {
      return Kind_Root;
   }

   void accept(DocVisitor *v) {
      CompAccept<DocRoot>::accept(this, v);
   }

   void parse();

   bool indent() const {
      return m_indent;
   }

   bool singleLine() const {
      return m_singleLine;
   }

   bool isEmpty() const {
      return m_children.isEmpty();
   }

 private:
   bool m_indent;
   bool m_singleLine;
};


#endif
