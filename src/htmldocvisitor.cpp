/*************************************************************************
 *
 * Copyright (C) 2014-2018 Barbara Geller & Ansel Sermersheim
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

#include <QDir>

#include <htmldocvisitor.h>

#include <config.h>
#include <docparser.h>
#include <dia.h>
#include <doxy_globals.h>
#include <dot.h>
#include <htmlgen.h>
#include <htmlentity.h>
#include <language.h>
#include <message.h>
#include <msc.h>
#include <outputgen.h>
#include <parse_base.h>
#include <plantuml.h>
#include <util.h>

static const int NUM_HTML_LIST_TYPES = 4;
static const char types[][NUM_HTML_LIST_TYPES] = {"1", "a", "i", "A"};

static QString convertIndexWordToAnchor(const QString &word)
{
   QString result = "a";

   for (auto c : word) {

      if ((c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || (c >= '0' && c <= '9') ||
            c == '-' || c == '.' || c == '_') {

         result += c;

      } else {
         result += QString(":%1").arg(c.unicode(), 2, 16, QChar('0'));

      }
   }

   return result;
}

static bool mustBeOutsideParagraph(DocNode *n)
{
   switch (n->kind()) {
      /* <ul> */
      case DocNode::Kind_HtmlList:
      case DocNode::Kind_SimpleList:
      case DocNode::Kind_AutoList:

      /* <dl> */
      case DocNode::Kind_SimpleSect:
      case DocNode::Kind_ParamSect:
      case DocNode::Kind_HtmlDescList:
      case DocNode::Kind_XRefItem:

      /* <table> */
      case DocNode::Kind_HtmlTable:

      /* <h?> */
      case DocNode::Kind_Section:
      case DocNode::Kind_HtmlHeader:

      /* \internal */
      case DocNode::Kind_Internal:

      /* <div> */
      case DocNode::Kind_Include:
      case DocNode::Kind_Image:
      case DocNode::Kind_SecRefList:

      /* <hr> */
      case DocNode::Kind_HorRuler:
      /* CopyDoc gets paragraph markers from the wrapping DocPara node,
       * but needs to insert them for all documentation being copied to
       * preserve formatting.
       */

      case DocNode::Kind_Copy:
      /* <blockquote> */

      case DocNode::Kind_HtmlBlockQuote:
      /* \parblock */

      case DocNode::Kind_ParBlock:
         return true;

      case DocNode::Kind_Verbatim: {
         DocVerbatim *dv = (DocVerbatim *)n;
         return dv->type() != DocVerbatim::HtmlOnly || dv->isBlock();
      }

      case DocNode::Kind_StyleChange:
         return ((DocStyleChange *)n)->style() == DocStyleChange::Preformatted ||
                ((DocStyleChange *)n)->style() == DocStyleChange::Div ||
                ((DocStyleChange *)n)->style() == DocStyleChange::Center;

      case DocNode::Kind_Formula:
         return !((DocFormula *)n)->isInline();

      default:
         break;
   }

   return false;
}

static QString htmlAttribsToString(const HtmlAttribList &attribs)
{
   QString result;

   for (auto att : attribs) {
      if (! att.value.isEmpty())  {
         // ignore attribute without values as they are not XHTML compliant

         result += " ";
         result += att.name;
         result += "=\"" + convertToXML(att.value) + "\"";
      }
   }
   return result;
}

HtmlDocVisitor::HtmlDocVisitor(QTextStream &t, CodeOutputInterface &ci, QSharedPointer<Definition> ctx)
   : DocVisitor(DocVisitor_Html), m_t(t), m_ci(ci), m_insidePre(false), m_hide(false), m_ctx(ctx)
{
   if (ctx) {
      m_langExt = ctx->getDefFileExtension();
   }
}

// visitor functions for leaf nodes
void HtmlDocVisitor::visit(DocWord *w)
{
   if (m_hide) {
      return;
   }

   filter(w->word());
}

void HtmlDocVisitor::visit(DocLinkedWord *w)
{
   if (m_hide) {
      return;
   }

   startLink(w->ref(), w->file(), w->relPath(), w->anchor(), w->tooltip());
   filter(w->word());
   endLink();
}

void HtmlDocVisitor::visit(DocWhiteSpace *w)
{
   if (m_hide) {
      return;
   }

   if (m_insidePre) {
      m_t << w->chars();
   } else {
      m_t << " ";
   }
}

void HtmlDocVisitor::visit(DocSymbol *s)
{
   if (m_hide) {
      return;
   }

   QString res = HtmlEntityMapper::instance()->html(s->symbol());

   if (! res.isEmpty()) {
      m_t << res;

   } else {
      err("Unsupported HTML entity found: %s\n", csPrintable(HtmlEntityMapper::instance()->html(s->symbol(), true)) );
   }
}

void HtmlDocVisitor::visit(DocURL *u)
{
   if (m_hide) {
      return;
   }

   if (u->isEmail()) {
      // mail address with no mailto

      QString url = "mailto:" + u->url();

      m_t << "<a href=\"" << convertToXML(url) << "\"" << ">" ;
      filter(u->url());
      m_t << "</a>";

   } else {
      // web address defined with no <a>

      m_t << "<a href=\"" << u->url() << "\">";
      filter(u->url());
      m_t << "</a>";
   }
}

void HtmlDocVisitor::visit(DocLineBreak *)
{
   if (m_hide) {
      return;
   }
   m_t << "<br />\n";
}

void HtmlDocVisitor::visit(DocHorRuler *hr)
{
   if (m_hide) {
      return;
   }

   forceEndParagraph(hr);
   m_t << "<hr/>\n";
   forceStartParagraph(hr);
}

void HtmlDocVisitor::visit(DocStyleChange *s)
{
   if (m_hide) {
      return;
   }

   switch (s->style()) {
      case DocStyleChange::Bold:
         if (s->enable()) {
            m_t << "<b" << htmlAttribsToString(s->attribs()) << ">";
         }      else {
            m_t << "</b>";
         }
         break;

      case DocStyleChange::Italic:
         if (s->enable()) {
            m_t << "<em" << htmlAttribsToString(s->attribs()) << ">";
         }     else {
            m_t << "</em>";
         }
         break;

      case DocStyleChange::Code:
         if (s->enable()) {
            m_t << "<code" << htmlAttribsToString(s->attribs()) << ">";
         }   else {
            m_t << "</code>";
         }
         break;

      case DocStyleChange::Subscript:
         if (s->enable()) {
            m_t << "<sub" << htmlAttribsToString(s->attribs()) << ">";
         }    else {
            m_t << "</sub>";
         }
         break;

      case DocStyleChange::Superscript:
         if (s->enable()) {
            m_t << "<sup" << htmlAttribsToString(s->attribs()) << ">";
         }    else {
            m_t << "</sup>";
         }
         break;

      case DocStyleChange::Center:
         if (s->enable()) {
            forceEndParagraph(s);
            m_t << "<center" << htmlAttribsToString(s->attribs()) << ">";
         } else {
            m_t << "</center>";
            forceStartParagraph(s);
         }
         break;

      case DocStyleChange::Small:
         if (s->enable()) {
            m_t << "<small" << htmlAttribsToString(s->attribs()) << ">";
         }  else {
            m_t << "</small>";
         }
         break;

      case DocStyleChange::Preformatted:
         if (s->enable()) {
            forceEndParagraph(s);
            m_t << "<pre" << htmlAttribsToString(s->attribs()) << ">";
            m_insidePre = true;

         } else {
            m_insidePre = false;
            m_t << "</pre>";
            forceStartParagraph(s);
         }
         break;

      case DocStyleChange::Div:
         if (s->enable()) {
            forceEndParagraph(s);
            m_t << "<div " << htmlAttribsToString(s->attribs()) << ">";

         } else {
            m_t << "</div>";
            forceStartParagraph(s);
         }
         break;

      case DocStyleChange::Span:
         if (s->enable()) {
            m_t << "<span" << htmlAttribsToString(s->attribs()) << ">";
         }  else {
            m_t << "</span>";
         }
         break;

   }
}

static void visitPreCaption(QTextStream &t, DocVerbatim *s)
{
   if (s->hasCaption()) {
      t << "<div class=\"caption\">" << endl;
   }
}

static void visitPostCaption(QTextStream &t, DocVerbatim *s)
{
   if (s->hasCaption()) {
      t << "</div>" << endl;
   }
}

static void visitCaption(HtmlDocVisitor *parent, QList<DocNode *> children)
{
   for (auto n : children) {
      n->accept(parent);
   }
}

void HtmlDocVisitor::visit(DocVerbatim *s)
{
   if (m_hide) {
      return;
   }

   QString lang = m_langExt;

   if (! s->language().isEmpty()) {
      // explicit language setting
      lang = s->language();
   }

   SrcLangExt langExt = getLanguageFromFileName(lang);

   switch (s->type()) {

      case DocVerbatim::Code:

         forceEndParagraph(s);
         m_t << PREFRAG_START;

         {
            auto tmp = Doxy_Globals::parserManager.getParser(lang);

            tmp->parseCode(m_ci, s->context(), s->text(), langExt,
                  s->isExample(), s->exampleFile(),
                  QSharedPointer<FileDef>(), -1, -1, false, QSharedPointer<MemberDef>(),
                  true, m_ctx);
         }

         m_t << PREFRAG_END;
         forceStartParagraph(s);

         break;

      case DocVerbatim::Verbatim:
         forceEndParagraph(s);
         m_t << /*PREFRAG_START <<*/ "<pre class=\"fragment\">";

         filter(s->text());

         m_t << "</pre>" /*<< PREFRAG_END*/;
         forceStartParagraph(s);

         break;

      case DocVerbatim::HtmlOnly:
         if (s->isBlock()) {
            forceEndParagraph(s);
         }

         m_t << s->text();
         if (s->isBlock()) {
            forceStartParagraph(s);
         }
         break;

      case DocVerbatim::ManOnly:
      case DocVerbatim::LatexOnly:
      case DocVerbatim::XmlOnly:
      case DocVerbatim::RtfOnly:
      case DocVerbatim::DocbookOnly:
         /* nothing */
         break;

      case DocVerbatim::Dot: {
         static int dotindex = 1;

         forceEndParagraph(s);

         QString fileName;
         fileName = QString("%1%2.dot").arg(Config::getString("html-output") + "/inline_dotgraph_").arg(dotindex++);

         QFile file(fileName);

         if (! file.open(QIODevice::WriteOnly)) {
            err("Unable to open file for writing %s, error: %d\n", csPrintable(fileName), file.error());

         } else {

            file.write(s->text().toUtf8());
            file.close();

            m_t << "<div class=\"dotgraph\">" << endl;

            writeDotFile(fileName, s->relPath(), s->context());

            visitPreCaption(m_t, s);
            visitCaption(this, s->children());
            visitPostCaption(m_t, s);

            m_t << "</div>" << endl;

            if (Config::getBool("dot-cleanup")) {
               file.remove();
            }
         }

         forceStartParagraph(s);

      }
      break;

      case DocVerbatim::Msc: {
         forceEndParagraph(s);

         static int mscindex = 1;

         QString baseName;
         baseName = QString("%1%2").arg(Config::getString("html-output") + "/inline_mscgraph_").arg(mscindex++);

         QFile file(baseName + ".msc");

         if (! file.open(QIODevice::WriteOnly)) {
            err("Unable to open file for writing %s.msc error: %d\n", csPrintable(baseName), file.error());

         } else {
            QString text = "msc {";
            text += s->text();
            text += "}";

            file.write( text.toUtf8() );
            file.close();

            m_t << "<div class=\"mscgraph\">" << endl;

            writeMscFile(baseName + ".msc", s->relPath(), s->context());

            visitPreCaption(m_t, s);
            visitCaption(this, s->children());
            visitPostCaption(m_t, s);

            m_t << "</div>" << endl;

            if (Config::getBool("dot-cleanup")) {
               file.remove();
            }
         }

         forceStartParagraph(s);
      }
      break;

      case DocVerbatim::PlantUML: {
         forceEndParagraph(s);

         static QString htmlOutput = Config::getString("html-output");
         QString baseName = writePlantUMLSource(htmlOutput, s->exampleFile(), s->text());

         m_t << "<div class=\"plantumlgraph\">" << endl;

         writePlantUMLFile(baseName, s->relPath(), s->context());

         visitPreCaption(m_t, s);
         visitCaption(this, s->children());
         visitPostCaption(m_t, s);

         m_t << "</div>" << endl;
         forceStartParagraph(s);
      }
      break;
   }
}

void HtmlDocVisitor::visit(DocAnchor *anc)
{
   if (m_hide) {
      return;
   }

   m_t << "<a class=\"anchor\" id=\"" << anc->anchor() << "\"></a>";
}

void HtmlDocVisitor::visit(DocInclude *inc)
{
   if (m_hide) {
      return;
   }

   SrcLangExt langExt = getLanguageFromFileName(inc->extension());

   switch (inc->type()) {

      case DocInclude::Include:
         forceEndParagraph(inc);
         m_t << PREFRAG_START;

         Doxy_Globals::parserManager.getParser(inc->extension())->parseCode(m_ci, inc->context(),
                     inc->text(), langExt, inc->isExample(), inc->exampleFile(),
                     QSharedPointer<FileDef>(), -1, -1, true, QSharedPointer<MemberDef>(), false, m_ctx);

         m_t << PREFRAG_END;
         forceStartParagraph(inc);
         break;

      case DocInclude::IncWithLines: {
         forceEndParagraph(inc);
         m_t << PREFRAG_START;

         QFileInfo cfi( inc->file() );
         QSharedPointer<FileDef> fd = QMakeShared<FileDef>(cfi.path(), cfi.fileName());

         Doxy_Globals::parserManager.getParser(inc->extension())->parseCode(m_ci, inc->context(),
                     inc->text(), langExt, inc->isExample(), inc->exampleFile(),
                     fd, -1, -1, false, QSharedPointer<MemberDef>(), true, m_ctx);

         m_t << PREFRAG_END;
         forceStartParagraph(inc);
      }
      break;

      case DocInclude::DontInclude:
         break;

      case DocInclude::HtmlInclude:
         m_t << inc->text();
         break;

      case DocInclude::LatexInclude:
         break;

      case DocInclude::VerbInclude:
         forceEndParagraph(inc);
         m_t << /*PREFRAG_START <<*/ "<pre class=\"fragment\">";
         filter(inc->text());
         m_t << "</pre>" /*<< PREFRAG_END*/;
         forceStartParagraph(inc);
         break;

      case DocInclude::Snippet: {
         forceEndParagraph(inc);
         m_t << PREFRAG_START;

         Doxy_Globals::parserManager.getParser(inc->extension())->parseCode(m_ci, inc->context(),
                     extractBlock(inc->text(), inc->blockId()), langExt, inc->isExample(), inc->exampleFile(),
                     QSharedPointer<FileDef>(), -1, -1, true, QSharedPointer<MemberDef>(), true, m_ctx);

         m_t << PREFRAG_END;
         forceStartParagraph(inc);
      }
      break;
   }
}

void HtmlDocVisitor::visit(DocIncOperator *op)
{
   if (op->isFirst()) {
      if (! m_hide) {
         m_t << PREFRAG_START;
      }

      pushEnabled();
      m_hide = true;
   }

   SrcLangExt langExt = getLanguageFromFileName(m_langExt);

   if (op->type() != DocIncOperator::Skip) {
      popEnabled();

      if (! m_hide) {
         Doxy_Globals::parserManager.getParser(m_langExt)->parseCode(m_ci, op->context(), op->text(),
            langExt, op->isExample(), op->exampleFile(),
            QSharedPointer<FileDef>(), -1, -1, false, QSharedPointer<MemberDef>(), true, m_ctx);
      }

      pushEnabled();
      m_hide = true;
   }

   if (op->isLast()) {
      popEnabled();
      if (! m_hide) {
         m_t << PREFRAG_END;
      }

   } else {
      if (! m_hide) {
         m_t << endl;
      }
   }
}

void HtmlDocVisitor::visit(DocFormula *f)
{
   if (m_hide) {
      return;
   }

   bool bDisplay = ! f->isInline();

   if (bDisplay) {
      forceEndParagraph(f);
      m_t << "<p class=\"formulaDsp\">" << endl;
   }

   if (Config::getBool("use-mathjax")) {
      QString text = f->text();
      bool closeInline = false;

      if (!bDisplay && ! text.isEmpty() && text.at(0) == '$' && text.at(text.length() - 1) == '$') {
         closeInline = true;
         text = text.mid(1, text.length() - 2);
         m_t << "\\(";
      }

      m_t << convertToHtml(text);
      if (closeInline) {
         m_t << "\\)";
      }

   } else {
      m_t << "<img class=\"formula" << (bDisplay ? "Dsp" : "Inl");
      m_t << "\" alt=\"";
      filterQuotedCdataAttr(f->text());

      m_t << "\"";

      // TODO: cache image dimensions on formula generation and give height/width
      // for faster preloading and better rendering of the page

      m_t << " src=\"" << f->relPath() << f->name() << ".png\"/>";

   }

   if (bDisplay) {
      m_t << endl << "</p>" << endl;
      forceStartParagraph(f);
   }
}

void HtmlDocVisitor::visit(DocIndexEntry *e)
{
   QString anchor = convertIndexWordToAnchor(e->entry());

   if (e->member()) {
      anchor.prepend(e->member()->anchor() + "_");
   }

   m_t << "<a name=\"" << anchor << "\"></a>";

   Doxy_Globals::indexList.addIndexItem(e->scope(), e->member(), anchor, e->entry());
}

void HtmlDocVisitor::visit(DocSimpleSectSep *)
{
   m_t << "</dd>" << endl;
   m_t << "<dd>" << endl;
}

void HtmlDocVisitor::visit(DocCite *cite)
{
   if (m_hide) {
      return;
   }

   if (! cite->file().isEmpty()) {
      startLink(cite->ref(), cite->file(), cite->relPath(), cite->anchor());
   } else {
      m_t << "<b>[";
   }

   filter(cite->text());

   if (!cite->file().isEmpty()) {
      endLink();
   } else {
      m_t << "]</b>";
   }
}

void HtmlDocVisitor::visitPre(DocAutoList *l)
{
   if (m_hide) {
      return;
   }

   forceEndParagraph(l);
   if (l->isEnumList()) {
      //
      // Do list type based on depth:
      // 1.
      //   a.
      //     i.
      //       A.
      //         1. (repeat)...
      //

      m_t << "<ol type=\"" << types[l->depth() % NUM_HTML_LIST_TYPES] << "\">";

   } else {
      m_t << "<ul>";
   }

   if (! l->isPreformatted()) {
      m_t << "\n";
   }
}

void HtmlDocVisitor::visitPost(DocAutoList *l)
{
   if (m_hide) {
      return;
   }

   if (l->isEnumList()) {
      m_t << "</ol>";
   } else {
      m_t << "</ul>";
   }

   if (!l->isPreformatted()) {
      m_t << "\n";
   }

   forceStartParagraph(l);
}

void HtmlDocVisitor::visitPre(DocAutoListItem *)
{
   if (m_hide) {
      return;
   }
   m_t << "<li>";
}

void HtmlDocVisitor::visitPost(DocAutoListItem *li)
{
   if (m_hide) {
      return;
   }
   m_t << "</li>";
   if (!li->isPreformatted()) {
      m_t << "\n";
   }
}

template<class T>
bool isFirstChildNode(T *parent, DocNode *node)
{
   return parent->children().first() == node;
}

template<class T>
bool isLastChildNode(T *parent, DocNode *node)
{
   return parent->children().last() == node;
}

bool isSeparatedParagraph(DocSimpleSect *parent, DocPara *par)
{
   QList<DocNode *> nodes = parent->children();

   int i = nodes.indexOf(par);

   if (i == -1) {
      return false;
   }

   int count = parent->children().count();
   if (count > 1 && i == 0) { // first node
      if (nodes.at(i + 1)->kind() == DocNode::Kind_SimpleSectSep) {
         return true;
      }

   } else if (count > 1 && i == count - 1) { // last node
      if (nodes.at(i - 1)->kind() == DocNode::Kind_SimpleSectSep) {
         return true;
      }

   } else if (count > 2 && i > 0 && i < count - 1) { // intermediate node
      if (nodes.at(i - 1)->kind() == DocNode::Kind_SimpleSectSep && nodes.at(i + 1)->kind() == DocNode::Kind_SimpleSectSep) {
         return true;
      }
   }
   return false;
}

static int getParagraphContext(DocPara *p, bool &isFirst, bool &isLast)
{
   int t   = 0;
   isFirst = false;
   isLast   = false;

   if (p && p->parent()) {

      switch (p->parent()->kind()) {
         case DocNode::Kind_ParBlock: {
            // hierarchy: node N -> para -> parblock -> para
            // adapt return value to kind of N

            DocNode::Kind kind = DocNode::Kind_Para;
            if ( p->parent()->parent() && p->parent()->parent()->parent() ) {
               kind = p->parent()->parent()->parent()->kind();
            }

            isFirst = isFirstChildNode((DocParBlock *)p->parent(), p);
            isLast = isLastChildNode ((DocParBlock *)p->parent(), p);
            t = 0;

            if (isFirst) {
               if (kind == DocNode::Kind_HtmlListItem ||
                     kind == DocNode::Kind_SecRefItem) {
                  t = 1;
               } else if (kind == DocNode::Kind_HtmlDescData ||
                          kind == DocNode::Kind_XRefItem ||
                          kind == DocNode::Kind_SimpleSect) {
                  t = 2;
               } else if (kind == DocNode::Kind_HtmlCell ||
                          kind == DocNode::Kind_ParamList) {
                  t = 5;
               }
            }
            if (isLast) {
               if (kind == DocNode::Kind_HtmlListItem ||
                     kind == DocNode::Kind_SecRefItem) {
                  t = 3;
               } else if (kind == DocNode::Kind_HtmlDescData ||
                          kind == DocNode::Kind_XRefItem ||
                          kind == DocNode::Kind_SimpleSect) {
                  t = 4;
               } else if (kind == DocNode::Kind_HtmlCell ||
                          kind == DocNode::Kind_ParamList) {
                  t = 6;
               }
            }
            break;
         }

         case DocNode::Kind_AutoListItem:
            isFirst = isFirstChildNode((DocAutoListItem *)p->parent(), p);
            isLast = isLastChildNode ((DocAutoListItem *)p->parent(), p);
            t = 1; // not used
            break;

         case DocNode::Kind_SimpleListItem:
            isFirst = true;
            isLast = true;
            t = 1; // not used
            break;

         case DocNode::Kind_ParamList:
            isFirst = true;
            isLast = true;
            t = 1; // not used
            break;

         case DocNode::Kind_HtmlListItem:
            isFirst = isFirstChildNode((DocHtmlListItem *)p->parent(), p);
            isLast = isLastChildNode ((DocHtmlListItem *)p->parent(), p);
            if (isFirst) {
               t = 1;
            }
            if (isLast) {
               t = 3;
            }
            break;
         case DocNode::Kind_SecRefItem:
            isFirst = isFirstChildNode((DocSecRefItem *)p->parent(), p);
            isLast = isLastChildNode ((DocSecRefItem *)p->parent(), p);
            if (isFirst) {
               t = 1;
            }
            if (isLast) {
               t = 3;
            }
            break;
         case DocNode::Kind_HtmlDescData:
            isFirst = isFirstChildNode((DocHtmlDescData *)p->parent(), p);
            isLast = isLastChildNode ((DocHtmlDescData *)p->parent(), p);
            if (isFirst) {
               t = 2;
            }
            if (isLast) {
               t = 4;
            }
            break;
         case DocNode::Kind_XRefItem:
            isFirst = isFirstChildNode((DocXRefItem *)p->parent(), p);
            isLast = isLastChildNode ((DocXRefItem *)p->parent(), p);
            if (isFirst) {
               t = 2;
            }
            if (isLast) {
               t = 4;
            }
            break;
         case DocNode::Kind_SimpleSect:
            isFirst = isFirstChildNode((DocSimpleSect *)p->parent(), p);
            isLast = isLastChildNode ((DocSimpleSect *)p->parent(), p);
            if (isFirst) {
               t = 2;
            }
            if (isLast) {
               t = 4;
            }
            if (isSeparatedParagraph((DocSimpleSect *)p->parent(), p))
               // if the paragraph is enclosed with separators it will
               // be included in <dd>..</dd> so avoid addition paragraph
               // markers
            {
               isFirst = isLast = true;
            }
            break;
         case DocNode::Kind_HtmlCell:
            isFirst = isFirstChildNode((DocHtmlCell *)p->parent(), p);
            isLast = isLastChildNode ((DocHtmlCell *)p->parent(), p);
            if (isFirst) {
               t = 5;
            }
            if (isLast) {
               t = 6;
            }
            break;
         default:
            break;
      }
      //printf("para=%p parent()->kind=%d isFirst=%d isLast=%d t=%d\n",
      //    p,p->parent()->kind(),isFirst,isLast,t);
   }
   return t;
}

void HtmlDocVisitor::visitPre(DocPara *p)
{
   if (m_hide) {
      return;
   }

   bool needsTag = false;
   if (p && p->parent()) {
      switch (p->parent()->kind()) {
         case DocNode::Kind_Section:
         case DocNode::Kind_Internal:
         case DocNode::Kind_HtmlListItem:
         case DocNode::Kind_HtmlDescData:
         case DocNode::Kind_HtmlCell:
         case DocNode::Kind_SimpleListItem:
         case DocNode::Kind_AutoListItem:
         case DocNode::Kind_SimpleSect:
         case DocNode::Kind_XRefItem:
         case DocNode::Kind_Copy:
         case DocNode::Kind_HtmlBlockQuote:
         case DocNode::Kind_ParBlock:
            needsTag = true;
            break;
         case DocNode::Kind_Root:
            needsTag = !((DocRoot *)p->parent())->singleLine();
            break;
         default:
            needsTag = false;
      }
   }

   // if the first element of a paragraph is something that should be outside of
   // the paragraph (<ul>,<dl>,<table>,..) then that will already started the
   // paragraph and we don't need to do it here
   uint nodeIndex = 0;

   if (p && nodeIndex < p->children().count()) {
      while (nodeIndex < p->children().count() &&
             p->children().at(nodeIndex)->kind() == DocNode::Kind_WhiteSpace) {
         nodeIndex++;
      }
      if (nodeIndex < p->children().count()) {
         DocNode *n = p->children().at(nodeIndex);

         if (mustBeOutsideParagraph(n)) {
            needsTag = false;
         }
      }
   }

   // check if this paragraph is the first or last child of a <li> or <dd>.
   // this allows us to mark the tag with a special class so we can
   // fix the otherwise ugly spacing.
   int t;

   static const char *contexts[7] = {
      "",                     // 0
      " class=\"startli\"",   // 1
      " class=\"startdd\"",   // 2
      " class=\"endli\"",     // 3
      " class=\"enddd\"",     // 4
      " class=\"starttd\"",   // 5
      " class=\"endtd\""      // 6
   };

   bool isFirst;
   bool isLast;

   t = getParagraphContext(p, isFirst, isLast);

   if (isFirst && isLast) {
      needsTag = false;
   }

   // write the paragraph tag (if needed)
   if (needsTag) {
      m_t << "<p" << contexts[t] << ">";
   }
}

void HtmlDocVisitor::visitPost(DocPara *p)
{
   bool needsTag = false;

   if (p && p->parent()) {

      switch (p->parent()->kind()) {
         case DocNode::Kind_Section:
         case DocNode::Kind_Internal:
         case DocNode::Kind_HtmlListItem:
         case DocNode::Kind_HtmlDescData:
         case DocNode::Kind_HtmlCell:
         case DocNode::Kind_SimpleListItem:
         case DocNode::Kind_AutoListItem:
         case DocNode::Kind_SimpleSect:
         case DocNode::Kind_XRefItem:
         case DocNode::Kind_Copy:
         case DocNode::Kind_HtmlBlockQuote:
         case DocNode::Kind_ParBlock:
            needsTag = true;
            break;

         case DocNode::Kind_Root:
            needsTag = !((DocRoot *)p->parent())->singleLine();
            break;

         default:
            needsTag = false;
      }
   }

   // if the last element of a paragraph is something that should be outside of
   // the paragraph (<ul>,<dl>,<table>) then that will already have ended the
   // paragraph and we don't need to do it here

   int nodeIndex = p->children().count() - 1;

   if (p && nodeIndex >= 0) {
      while (nodeIndex >= 0 && p->children().at(nodeIndex)->kind() == DocNode::Kind_WhiteSpace) {
         nodeIndex--;
      }
      if (nodeIndex >= 0) {
         DocNode *n = p->children().at(nodeIndex);
         if (mustBeOutsideParagraph(n)) {
            needsTag = false;
         }
      }
   }

   bool isFirst;
   bool isLast;

   getParagraphContext(p, isFirst, isLast);

   if (isFirst && isLast) {
      needsTag = false;
   }

   if (needsTag) {
      m_t << "</p>\n";
   }
}

void HtmlDocVisitor::visitPre(DocRoot *)
{
}

void HtmlDocVisitor::visitPost(DocRoot *)
{
}

void HtmlDocVisitor::visitPre(DocSimpleSect *s)
{
   if (m_hide) {
      return;
   }

   forceEndParagraph(s);

   m_t << "<dl class=\"section " << s->typeString() << "\"><dt>";

   switch (s->type()) {
      case DocSimpleSect::See:
         m_t << theTranslator->trSeeAlso();
         break;

      case DocSimpleSect::Return:
         m_t << theTranslator->trReturns();
         break;

      case DocSimpleSect::Author:
         m_t << theTranslator->trAuthor(true, true);
         break;

      case DocSimpleSect::Authors:
         m_t << theTranslator->trAuthor(true, false);
         break;

      case DocSimpleSect::Version:
         m_t << theTranslator->trVersion();
         break;

      case DocSimpleSect::Since:
         m_t << theTranslator->trSince();
         break;

      case DocSimpleSect::Date:
         m_t << theTranslator->trDate();
         break;

      case DocSimpleSect::Note:
         m_t << theTranslator->trNote();
         break;

      case DocSimpleSect::Warning:
         m_t << theTranslator->trWarning();
         break;

      case DocSimpleSect::Pre:
         m_t << theTranslator->trPrecondition();
         break;

      case DocSimpleSect::Post:
         m_t << theTranslator->trPostcondition();
         break;

      case DocSimpleSect::Copyright:
         m_t << theTranslator->trCopyright();
         break;

      case DocSimpleSect::Invar:
         m_t << theTranslator->trInvariant();
         break;

      case DocSimpleSect::Remark:
         m_t << theTranslator->trRemarks();
         break;

      case DocSimpleSect::Attention:
         m_t << theTranslator->trAttention();
         break;

      case DocSimpleSect::User:
         break;

      case DocSimpleSect::Rcs:
         break;

      case DocSimpleSect::Unknown:
         break;
   }

   // special case 1: user defined title
   if (s->type() != DocSimpleSect::User && s->type() != DocSimpleSect::Rcs) {
      m_t << "</dt><dd>";
   }
}

void HtmlDocVisitor::visitPost(DocSimpleSect *s)
{
   if (m_hide) {
      return;
   }

   m_t << "</dd></dl>\n";
   forceStartParagraph(s);
}

void HtmlDocVisitor::visitPre(DocTitle *)
{
}

void HtmlDocVisitor::visitPost(DocTitle *)
{
   if (m_hide) {
      return;
   }
   m_t << "</dt><dd>";
}

void HtmlDocVisitor::visitPre(DocSimpleList *sl)
{
   if (m_hide) {
      return;
   }

   forceEndParagraph(sl);
   m_t << "<ul>";

   if (! sl->isPreformatted()) {
      m_t << "\n";
   }

}

void HtmlDocVisitor::visitPost(DocSimpleList *sl)
{
   if (m_hide) {
      return;
   }
   m_t << "</ul>";
   if (!sl->isPreformatted()) {
      m_t << "\n";
   }
   forceStartParagraph(sl);
}

void HtmlDocVisitor::visitPre(DocSimpleListItem *)
{
   if (m_hide) {
      return;
   }
   m_t << "<li>";
}

void HtmlDocVisitor::visitPost(DocSimpleListItem *li)
{
   if (m_hide) {
      return;
   }
   m_t << "</li>";
   if (!li->isPreformatted()) {
      m_t << "\n";
   }
}

void HtmlDocVisitor::visitPre(DocSection *s)
{
   if (m_hide) {
      return;
   }

   forceEndParagraph(s);

   m_t << "<h" << s->level() << ">";
   m_t << "<a class=\"anchor\" id=\"" << s->anchor();
   m_t << "\"></a>" << endl;

   filter(convertCharEntities(s->title()));

   m_t << "</h" << s->level() << ">\n";
}

void HtmlDocVisitor::visitPost(DocSection *s)
{
   forceStartParagraph(s);
}

void HtmlDocVisitor::visitPre(DocHtmlList *s)
{
   if (m_hide) {
      return;
   }

   forceEndParagraph(s);
   if (s->type() == DocHtmlList::Ordered) {
      m_t << "<ol" << htmlAttribsToString(s->attribs()) << ">\n";

   } else {
      m_t << "<ul" << htmlAttribsToString(s->attribs()) << ">\n";

   }
}

void HtmlDocVisitor::visitPost(DocHtmlList *s)
{
   if (m_hide) {
      return;
   }

   if (s->type() == DocHtmlList::Ordered) {
      m_t << "</ol>";
   } else {
      m_t << "</ul>";
   }

   if (! s->isPreformatted()) {
      m_t << "\n";
   }
   forceStartParagraph(s);
}

void HtmlDocVisitor::visitPre(DocHtmlListItem *i)
{
   if (m_hide) {
      return;
   }

   m_t << "<li" << htmlAttribsToString(i->attribs()) << ">";

   if (! i->isPreformatted()) {
      m_t << "\n";
   }
}

void HtmlDocVisitor::visitPost(DocHtmlListItem *)
{
   if (m_hide) {
      return;
   }
   m_t << "</li>\n";
}

void HtmlDocVisitor::visitPre(DocHtmlDescList *dl)
{
   if (m_hide) {
      return;
   }

   forceEndParagraph(dl);
   m_t << "<dl" << htmlAttribsToString(dl->attribs()) << ">\n";
}

void HtmlDocVisitor::visitPost(DocHtmlDescList *dl)
{
   if (m_hide) {
      return;
   }

   m_t << "</dl>\n";
   forceStartParagraph(dl);
}

void HtmlDocVisitor::visitPre(DocHtmlDescTitle *dt)
{
   if (m_hide) {
      return;
   }

   m_t << "<dt" << htmlAttribsToString(dt->attribs()) << ">";
}

void HtmlDocVisitor::visitPost(DocHtmlDescTitle *)
{
   if (m_hide) {
      return;
   }
   m_t << "</dt>\n";
}

void HtmlDocVisitor::visitPre(DocHtmlDescData *dd)
{
   if (m_hide) {
      return;
   }
   m_t << "<dd" << htmlAttribsToString(dd->attribs()) << ">";
}

void HtmlDocVisitor::visitPost(DocHtmlDescData *)
{
   if (m_hide) {
      return;
   }
   m_t << "</dd>\n";
}

void HtmlDocVisitor::visitPre(DocHtmlTable *t)
{
   if (m_hide) {
      return;
   }

   forceEndParagraph(t);

   if (t->hasCaption()) {
      m_t << "<a class=\"anchor\" id=\"" << t->caption()->anchor() << "\"></a>\n";
   }

   QString attrs = htmlAttribsToString(t->attribs());

   if (attrs.isEmpty()) {
      m_t << "<table class=\"doxtable\">\n";
   } else {
      m_t << "<table " << htmlAttribsToString(t->attribs()) << ">\n";
   }
}

void HtmlDocVisitor::visitPost(DocHtmlTable *t)
{
   if (m_hide) {
      return;
   }

   m_t << "</table>\n";
   forceStartParagraph(t);
}

void HtmlDocVisitor::visitPre(DocHtmlRow *tr)
{
   if (m_hide) {
      return;
   }

   m_t << "<tr" << htmlAttribsToString(tr->attribs()) << ">\n";
}

void HtmlDocVisitor::visitPost(DocHtmlRow *)
{
   if (m_hide) {
      return;
   }

   m_t << "</tr>\n";
}

void HtmlDocVisitor::visitPre(DocHtmlCell *c)
{
   if (m_hide) {
      return;
   }

   if (c->isHeading()) {
      m_t << "<th" << htmlAttribsToString(c->attribs()) << ">";
   } else {
      m_t << "<td" << htmlAttribsToString(c->attribs()) << ">";
   }
}

void HtmlDocVisitor::visitPost(DocHtmlCell *c)
{
   if (m_hide) {
      return;
   }
   if (c->isHeading()) {
      m_t << "</th>";
   } else {
      m_t << "</td>";
   }
}

void HtmlDocVisitor::visitPre(DocHtmlCaption *c)
{
   if (m_hide) {
      return;
   }

   m_t << "<caption" << htmlAttribsToString(c->attribs());
   m_t << ">";
}

void HtmlDocVisitor::visitPost(DocHtmlCaption *)
{
   if (m_hide) {
      return;
   }

   m_t << "</caption>\n";
}

void HtmlDocVisitor::visitPre(DocInternal *)
{
   if (m_hide) {
      return;
   }
}

void HtmlDocVisitor::visitPost(DocInternal *)
{
   if (m_hide) {
      return;
   }
}

void HtmlDocVisitor::visitPre(DocHRef *href)
{
   if (m_hide) {
      return;
   }

   if (href->url().left(7) == "mailto:") {
      QString url = href->url();
      m_t << "<a href=\"" << convertToXML(url) << "\"" << htmlAttribsToString(href->attribs()) << ">";

   } else {
      QString url = correctURL(href->url(), href->relPath());
      m_t << "<a href=\"" << convertToXML(url) << "\"" << htmlAttribsToString(href->attribs()) << ">";
   }
}

void HtmlDocVisitor::visitPost(DocHRef *)
{
   if (m_hide) {
      return;
   }
   m_t << "</a>";
}

void HtmlDocVisitor::visitPre(DocHtmlHeader *header)
{
   if (m_hide) {
      return;
   }

   forceEndParagraph(header);
   m_t << "<h" << header->level()
       << htmlAttribsToString(header->attribs()) << ">";
}

void HtmlDocVisitor::visitPost(DocHtmlHeader *header)
{
   if (m_hide) {
      return;
   }

   m_t << "</h" << header->level() << ">\n";
   forceStartParagraph(header);
}

void HtmlDocVisitor::visitPre(DocImage *img)
{
   if (img->type() == DocImage::Html) {
      // CopperSpice: do not force    04/19/2015
      // forceEndParagraph(img);

      if (m_hide) {
         return;
      }

      QString baseName = img->name();
      int i;

      if ((i = baseName.lastIndexOf('/')) != -1 || (i = baseName.lastIndexOf('\\')) != -1) {
         baseName = baseName.right(baseName.length() - i - 1);
      }

      m_t << "<div class=\"image\">" << endl;

      QString sizeAttribs;

      if (! img->width().isEmpty()) {
         sizeAttribs += " width=\"" + img->width() + "\"";
      }

      if (! img->height().isEmpty()) {
         sizeAttribs += " height=\"" + img->height() + "\"";
      }

      QString url = img->url();

      if (url.isEmpty()) {

         if (img->name().endsWith(".svg"))  {
            m_t << "<object type=\"image/svg+xml\" data=\"" << img->relPath() << img->name()
                  << "\"" << sizeAttribs << htmlAttribsToString(img->attribs()) << ">" << baseName
                  << "</object>" << endl;

         } else {
            m_t << "<img src=\"" << img->relPath() << img->name() << "\" alt=\""
                << baseName << "\"" << sizeAttribs << htmlAttribsToString(img->attribs())
                << "/>" << endl;
         }

      } else {

         if (img->name().endsWith(".svg"))  {

            m_t << "<object type=\"image/svg+xml\" data=\"" << correctURL(url,img->relPath())
                << "\"" << sizeAttribs << htmlAttribsToString(img->attribs()) << "></object>" << endl;

         } else {
            m_t << "<img src=\"" << correctURL(url, img->relPath()) << "\" "
                << sizeAttribs << htmlAttribsToString(img->attribs())
                << "/>" << endl;
         }
      }

      if (img->hasCaption()) {
         m_t << "<div class=\"caption\">" << endl;
      }

   } else {
      // other format -> skip
      pushEnabled();
      m_hide = true;
   }
}

void HtmlDocVisitor::visitPost(DocImage *img)
{
   if (img->type() == DocImage::Html) {
      if (m_hide) {
         return;
      }

      if (img->hasCaption()) {
         m_t << "</div>";
      }

      m_t << "</div>" << endl;

      // CopperSpice: do not force    04/19/2015
      // forceStartParagraph(img);

   } else { // other format
      popEnabled();
   }
}

void HtmlDocVisitor::visitPre(DocDotFile *df)
{
   if (m_hide) {
      return;
   }

   m_t << "<div class=\"dotgraph\">" << endl;
   writeDotFile(df->file(), df->relPath(), df->context());

   if (df->hasCaption()) {
      m_t << "<div class=\"caption\">" << endl;
   }
}

void HtmlDocVisitor::visitPost(DocDotFile *df)
{
   if (m_hide) {
      return;
   }
   if (df->hasCaption()) {
      m_t << "</div>" << endl;
   }
   m_t << "</div>" << endl;
}

void HtmlDocVisitor::visitPre(DocMscFile *df)
{
   if (m_hide) {
      return;
   }
   m_t << "<div class=\"mscgraph\">" << endl;
   writeMscFile(df->file(), df->relPath(), df->context());
   if (df->hasCaption()) {
      m_t << "<div class=\"caption\">" << endl;
   }
}
void HtmlDocVisitor::visitPost(DocMscFile *df)
{
   if (m_hide) {
      return;
   }
   if (df->hasCaption()) {
      m_t << "</div>" << endl;
   }
   m_t << "</div>" << endl;
}

void HtmlDocVisitor::visitPre(DocDiaFile *df)
{
   if (m_hide) {
      return;
   }

   m_t << "<div class=\"diagraph\">" << endl;
   writeDiaFile(df->file(), df->relPath(), df->context());

   if (df->hasCaption()) {
      m_t << "<div class=\"caption\">" << endl;
   }
}
void HtmlDocVisitor::visitPost(DocDiaFile *df)
{
   if (m_hide) {
      return;
   }
   if (df->hasCaption()) {
      m_t << "</div>" << endl;
   }
   m_t << "</div>" << endl;
}

void HtmlDocVisitor::visitPre(DocLink *lnk)
{
   if (m_hide) {
      return;
   }

   startLink(lnk->ref(), lnk->file(), lnk->relPath(), lnk->anchor());
}

void HtmlDocVisitor::visitPost(DocLink *)
{
   if (m_hide) {
      return;
   }
   endLink();
}

void HtmlDocVisitor::visitPre(DocRef *ref)
{
   if (m_hide) {
      return;
   }

   if (! ref->file().isEmpty()) {
      // if ref->isSubPage(), HTML use ref->file(), Latex and RTF use ref->anchor()

      startLink(ref->ref(), ref->file(), ref->relPath(), ref->isSubPage() ? QString() : ref->anchor());
   }

   if (! ref->hasLinkText()) {
      filter(ref->targetTitle());
   }
}

void HtmlDocVisitor::visitPost(DocRef *ref)
{
   if (m_hide) {
      return;
   }

   if (! ref->file().isEmpty()) {
      endLink();
   }
}

void HtmlDocVisitor::visitPre(DocSecRefItem *ref)
{
   if (m_hide) {
      return;
   }
   QString refName = ref->file();

   if (refName.right(Doxy_Globals::htmlFileExtension.length()) != QString(Doxy_Globals::htmlFileExtension)) {
      refName += Doxy_Globals::htmlFileExtension;
   }

   m_t << "<li><a href=\"" << refName << "#" << ref->anchor() << "\">";

}

void HtmlDocVisitor::visitPost(DocSecRefItem *)
{
   if (m_hide) {
      return;
   }
   m_t << "</a></li>\n";
}

void HtmlDocVisitor::visitPre(DocSecRefList *s)
{
   if (m_hide) {
      return;
   }

   forceEndParagraph(s);
   m_t << "<div class=\"multicol\">" << endl;
   m_t << "<ul>" << endl;
}

void HtmlDocVisitor::visitPost(DocSecRefList *s)
{
   if (m_hide) {
      return;
   }

   m_t << "</ul>" << endl;
   m_t << "</div>" << endl;
   forceStartParagraph(s);
}

//void HtmlDocVisitor::visitPre(DocLanguage *l)
//{
//  QString langId = Config::getEnum("output-language");
//  if (l->id().toLower()!=langId.lower())
//  {
//    pushEnabled();
//    m_hide = true;
//  }
//}

//void HtmlDocVisitor::visitPost(DocLanguage *l)
//{
//  QString langId = Config::getEnum("output-language");
//  if (l->id().toLower()!=langId.lower())
//  {
//    popEnabled();
//  }
//}

void HtmlDocVisitor::visitPre(DocParamSect *s)
{
   if (m_hide) {
      return;
   }

   forceEndParagraph(s);

   QString className;
   QString heading;

   switch (s->type()) {
      case DocParamSect::Param:
         heading = theTranslator->trParameters();
         className = "params";
         break;

      case DocParamSect::RetVal:
         heading = theTranslator->trReturnValues();
         className = "retval";
         break;

      case DocParamSect::Exception:
         heading = theTranslator->trExceptions();
         className = "exception";
         break;

      case DocParamSect::TemplateParam:
         heading = theTranslator->trTemplateParameters();
         className = "tparams";
         break;

      default:
         assert(0);
   }

   m_t << "<dl class=\"" << className << "\"><dt>";
   m_t << heading;
   m_t << "</dt><dd>" << endl;
   m_t << "  <table class=\"" << className << "\">" << endl;
}

void HtmlDocVisitor::visitPost(DocParamSect *s)
{
   if (m_hide) {
      return;
   }

   m_t << "  </table>" << endl;
   m_t << "  </dd>" << endl;
   m_t << "</dl>" << endl;

   forceStartParagraph(s);
}

void HtmlDocVisitor::visitPre(DocParamList *pl)
{
   if (m_hide) {
      return;
   }

   m_t << "    <tr>";
   DocParamSect *sect = 0;

   if (pl->parent()->kind() == DocNode::Kind_ParamSect) {
      sect = (DocParamSect *)pl->parent();
   }

   if (sect && sect->hasInOutSpecifier()) {
      m_t << "<td class=\"paramdir\">";
      if (pl->direction() != DocParamSect::Unspecified) {
         m_t << "[";
         if (pl->direction() == DocParamSect::In) {
            m_t << "in";
         } else if (pl->direction() == DocParamSect::Out) {
            m_t << "out";
         } else if (pl->direction() == DocParamSect::InOut) {
            m_t << "in,out";
         }
         m_t << "]";
      }
      m_t << "</td>";
   }

   if (sect && sect->hasTypeSpecifier()) {
      m_t << "<td class=\"paramtype\">";

      bool first = true;

      for (auto type : pl->paramTypes()) {
         if (!first) {
            m_t << "&#160;|&#160;";

         } else {
            first = false;
         }

         if (type->kind() == DocNode::Kind_Word) {
            visit((DocWord *)type);

         } else if (type->kind() == DocNode::Kind_LinkedWord) {
            visit((DocLinkedWord *)type);
         }
      }

      m_t << "</td>";
   }

   m_t << "<td class=\"paramname\">";

   bool first = true;

   for (auto param : pl->parameters()) {
      if (! first) {
         m_t << ",";
      } else {
         first = false;
      }

      if (param->kind() == DocNode::Kind_Word) {
         visit((DocWord *)param);

      } else if (param->kind() == DocNode::Kind_LinkedWord) {
         visit((DocLinkedWord *)param);
      }
   }

   m_t << "</td><td>";
}

void HtmlDocVisitor::visitPost(DocParamList *)
{
   if (m_hide) {
      return;
   }

   m_t << "</td></tr>" << endl;
}

void HtmlDocVisitor::visitPre(DocXRefItem *x)
{
   if (m_hide) {
      return;
   }
   if (x->title().isEmpty()) {
      return;
   }

   forceEndParagraph(x);
   bool anonymousEnum = x->file() == "@";

   if (! anonymousEnum) {
      m_t << "<dl class=\"" << x->key() << "\"><dt><b><a class=\"el\" href=\""
          << x->relPath() << x->file() << Doxy_Globals::htmlFileExtension
          << "#" << x->anchor() << "\">";

   } else {
      m_t << "<dl class=\"" << x->key() << "\"><dt><b>";
   }

   filter(x->title());
   m_t << ":";

   if (!anonymousEnum) {
      m_t << "</a>";
   }

   m_t << "</b></dt><dd>";
}

void HtmlDocVisitor::visitPost(DocXRefItem *x)
{
   if (m_hide) {
      return;
   }
   if (x->title().isEmpty()) {
      return;
   }
   m_t << "</dd></dl>" << endl;
   forceStartParagraph(x);
}

void HtmlDocVisitor::visitPre(DocInternalRef *ref)
{
   if (m_hide) {
      return;
   }

   startLink(0, ref->file(), ref->relPath(), ref->anchor());
}

void HtmlDocVisitor::visitPost(DocInternalRef *)
{
   if (m_hide) {
      return;
   }
   endLink();
   m_t << " ";
}

void HtmlDocVisitor::visitPre(DocCopy *)
{
}

void HtmlDocVisitor::visitPost(DocCopy *)
{
}

void HtmlDocVisitor::visitPre(DocText *)
{
}

void HtmlDocVisitor::visitPost(DocText *)
{
}

void HtmlDocVisitor::visitPre(DocHtmlBlockQuote *b)
{
   if (m_hide) {
      return;
   }

   forceEndParagraph(b);

   QString attrs = htmlAttribsToString(b->attribs());

   if (attrs.isEmpty()) {
      m_t << "<blockquote class=\"doxtable\">\n";
   } else {
      m_t << "<blockquote " << htmlAttribsToString(b->attribs()) << ">\n";
   }
}

void HtmlDocVisitor::visitPost(DocHtmlBlockQuote *b)
{
   if (m_hide) {
      return;
   }

   m_t << "</blockquote>" << endl;
   forceStartParagraph(b);
}

void HtmlDocVisitor::visitPre(DocParBlock *)
{
   if (m_hide) {
      return;
   }
}

void HtmlDocVisitor::visitPost(DocParBlock *)
{
   if (m_hide) {
      return;
   }
}

void HtmlDocVisitor::filter(const QString &str)
{
   if (str.isEmpty()) {
      return;
   }

   QString result = str;

   if (result.contains("$tr") )  {
      // used in the doxypress test build to show sample translations

      result = result.replace("$trPublicTypedefs",           theTranslator->trPublicTypedefs());
      result = result.replace("$trProtectedMembers",         theTranslator->trProtectedMembers());
      result = result.replace("$trPrivateSlots",             theTranslator->trPrivateSlots());
      result = result.replace("$trConstructorDocumentation", theTranslator->trConstructorDocumentation());
      result = result.replace("$trModulesDescription",       theTranslator->trModulesDescription());
      result = result.replace("$trDeprecatedList",           theTranslator->trDeprecatedList());
      result = result.replace("$trLegendDocs",               theTranslator->trLegendDocs("png"));
   }

   for (auto c : result) {

      switch (c.unicode()) {
         case '<':
            m_t << "&lt;";
            break;

         case '>':
            m_t << "&gt;";
            break;

         case '&':
            m_t << "&amp;";
            break;

         default:
            m_t << c;
      }
   }
}

// Escape basic entities to produce a valid CDATA attribute value,
// assume that the outer quoting will be using the double quote &quot;
void HtmlDocVisitor::filterQuotedCdataAttr(const QString &str)
{
   if (str.isEmpty()) {
      return;
   }

   for (auto c : str) {

      switch (c.unicode()) {
         case '&':
            m_t << "&amp;";
            break;

         case '"':
            m_t << "&quot;";
            break;

         case '<':
            m_t << "&lt;";
            break;

         case '>':
            m_t << "&gt;";
            break;

         default:
            m_t << c;
      }
   }
}

void HtmlDocVisitor::startLink(const QString &ref, const QString &file, const QString &relPath,
                  const QString &anchor, const QString &tooltip)
{
   if (! ref.isEmpty()) {
      // link to entity imported via tag file
      m_t << "<a class=\"elRef\" ";
      m_t << externalLinkTarget() << externalRef(relPath, ref, false);

   } else {
      // local link
      m_t << "<a class=\"el\" ";
   }

   m_t << "href=\"";
   m_t << externalRef(relPath, ref, true);


   if (! file.isEmpty()) {
      m_t << file << Doxy_Globals::htmlFileExtension;
   }

   if (! anchor.isEmpty()) {
      m_t << "#" << anchor;
   }

   m_t << "\"";

   if (! tooltip.isEmpty()) {
      m_t << " title=\"" << substitute(tooltip, "\"", "&quot;") << "\"";
   }

   m_t << ">";
}

void HtmlDocVisitor::endLink()
{
   m_t << "</a>";
}

void HtmlDocVisitor::pushEnabled()
{
   m_enabled.push(m_hide);
}

void HtmlDocVisitor::popEnabled()
{
   m_hide = m_enabled.pop();
}

void HtmlDocVisitor::writeDotFile(const QString &fn, const QString &relPath, const QString &context)
{
   QString baseName = fn;
   int i;

   if ((i = baseName.lastIndexOf('/')) != -1) {
      baseName = baseName.right(baseName.length() - i - 1);
   }

   if ((i = baseName.indexOf('.')) != -1) {
      // strip extension
      baseName = baseName.left(i);
   }

   baseName.prepend("dot_");
   QString outDir = Config::getString("html-output");

   writeDotGraphFromFile(fn, outDir, baseName, GOF_BITMAP);
   writeDotImageMapFromFile(m_t, fn, outDir, relPath, baseName, context);
}

void HtmlDocVisitor::writeMscFile(const QString &fileName, const QString &relPath, const QString &context)
{
   QString baseName = fileName;
   int i;

   if ((i = baseName.lastIndexOf('/')) != -1) {
      // strip path
      baseName = baseName.right(baseName.length() - i - 1);
   }

   if ((i = baseName.indexOf('.')) != -1) {
      // strip extension
      baseName = baseName.left(i);
   }

   baseName.prepend("msc_");

   static const QString outDir   = Config::getString("html-output");
   static const QString imageExt = Config::getEnum("dot-image-extension");

   MscOutputFormat mscFormat = MSC_BITMAP;

   if (imageExt == "svg") {
      mscFormat = MSC_SVG;
   }

   writeMscGraphFromFile(fileName, outDir, baseName, mscFormat);
   writeMscImageMapFromFile(m_t, fileName, outDir, relPath, baseName, context, mscFormat);
}

void HtmlDocVisitor::writeDiaFile(const QString &fileName, const QString &relPath, const QString &)
{
   QString baseName = fileName;
   int i;

   if ((i = baseName.lastIndexOf('/')) != -1) {
      // strip path
      baseName = baseName.right(baseName.length() - i - 1);
   }

   if ((i = baseName.indexOf('.')) != -1) {
      // strip extension
      baseName = baseName.left(i);
   }

   baseName.prepend("dia_");

   QString outDir = Config::getString("html-output");
   writeDiaGraphFromFile(fileName, outDir, baseName, DIA_BITMAP);

   m_t << "<img src=\"" << relPath << baseName << ".png" << "\" />" << endl;
}

void HtmlDocVisitor::writePlantUMLFile(const QString &fileName, const QString &relPath, const QString &)
{
   QString baseName = fileName;
   int i;

   if ((i = baseName.lastIndexOf('/')) != -1) {
      // strip path
      baseName = baseName.right(baseName.length() - i - 1);
   }

   if ((i = baseName.lastIndexOf('.')) != -1) {
      // strip extension
      baseName = baseName.left(i);
   }

   static const QString outDir   = Config::getString("html-output");
   static const QString imageExt = Config::getEnum("dot-image-extension");

   if (imageExt == "svg") {
      generatePlantUMLOutput(fileName, outDir, PUML_SVG);

      // m_t << "<iframe scrolling=\"no\" frameborder=\"0\" src=\"" << relPath << baseName
      //     << ".svg" << "\" />" << endl;
      // m_t << "<p><b>Yourbrowser is unable to show SVG: try Firefox, Chrome, Safari, or Opera instead</b></p>";
      // m_t << "</iframe>" << endl;

      m_t << "<object type=\"image/svg+xml\" data=\"" << relPath << baseName << ".svg\"></object>" << endl;

   } else {
      generatePlantUMLOutput(fileName, outDir, PUML_BITMAP);
      m_t << "<img src=\"" << relPath << baseName << ".png" << "\" />" << endl;
   }
}

/** Returns TRUE if the child nodes in paragraph \a para until \a nodeIndex
    contain a style change node that is still active and that style change is one that
    must be located outside of a paragraph, i.e. it is a center, div, or pre tag.
 */
static bool insideStyleChange_OutsidePara(DocPara *para,int nodeIndex)
{
   int styleMask = 0;
   bool styleOutsideParagraph = false;

   while (nodeIndex >= 0 && ! styleOutsideParagraph) {
      DocNode *n = para->children().at(nodeIndex);

      if (n->kind() == DocNode::Kind_StyleChange) {
         DocStyleChange *sc = dynamic_cast<DocStyleChange*>(n);

         if (! sc->enable()) {
            // remember styles which have been closed already
            styleMask |= sc->style();
         }

         bool paraStyle = (sc->style() == DocStyleChange::Center ||
                  sc->style() == DocStyleChange::Div || sc->style() == DocStyleChange::Preformatted);

         if (sc->enable() && (styleMask&(int)sc->style()) == 0 && paraStyle) {
            // style change that is still active
            styleOutsideParagraph = true;
         }
      }

      nodeIndex--;
   }

   return styleOutsideParagraph;
}

/** Used for items found inside a paragraph, which due to XHTML restrictions
 *  have to be outside of the paragraph. This method will force the end of
 *  the current paragraph and forceStartParagraph() will restart it.
 */
void HtmlDocVisitor::forceEndParagraph(DocNode *n)
{
   if (n->parent() && n->parent()->kind() == DocNode::Kind_Para) {

      DocPara *para = (DocPara *)n->parent();
      int nodeIndex = para->children().indexOf(n);

      nodeIndex--;
      if (nodeIndex < 0) {
         // first node
         return;
      }

      while (nodeIndex >= 0 && para->children().at(nodeIndex)->kind() == DocNode::Kind_WhiteSpace) {
         nodeIndex--;
      }

      if (nodeIndex >= 0) {
         DocNode *n = para->children().at(nodeIndex);

         if (mustBeOutsideParagraph(n)) {
            return;
         }
      }

      nodeIndex--;
      bool styleOutsideParagraph = insideStyleChange_OutsidePara(para, nodeIndex);

      bool isFirst;
      bool isLast;

      getParagraphContext(para, isFirst, isLast);

      if (isFirst && isLast) {
         return;
      }

      if (styleOutsideParagraph) {
         return;
      }

      m_t << "</p>";
   }
}

/** Used for items found inside a paragraph, which due to XHTML restrictions
 *  have to be outside of the paragraph. This method will force the start of
 *  the paragraph, that was previously ended by forceEndParagraph().
 */
void HtmlDocVisitor::forceStartParagraph(DocNode *n)
{
   if (n->parent() && n->parent()->kind() == DocNode::Kind_Para) {
      // if we are inside a paragraph

      DocPara *para = (DocPara *)n->parent();
      int nodeIndex = para->children().indexOf(n);
      int numNodes  = para->children().count();

      bool styleOutsideParagraph = insideStyleChange_OutsidePara(para,nodeIndex);
      if (styleOutsideParagraph) {
         return;
      }

      nodeIndex++;

      if (nodeIndex == numNodes) {
         // last node
         return;
      }

      while (nodeIndex < numNodes && para->children().at(nodeIndex)->kind() == DocNode::Kind_WhiteSpace) {
         nodeIndex++;
      }

      if (nodeIndex < numNodes) {
         DocNode *n = para->children().at(nodeIndex);

         if (mustBeOutsideParagraph(n)) {
            return;
         }

      } else {
         // only whitespace at the end
         return;

      }

      bool isFirst;
      bool isLast;

      getParagraphContext(para, isFirst, isLast);

      if (isFirst && isLast) {
         return;
      }

      m_t << "<p>";
   }
}

