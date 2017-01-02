/*************************************************************************
 *
 * Copyright (C) 2014-2017 Barbara Geller & Ansel Sermersheim 
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
#include <QHash>

#include <stdio.h>

#include <htags.h>

#include <config.h>
#include <message.h>
#include <portable.h>
#include <util.h>

bool Htags::useHtags = false;

static QDir g_inputDir;
static QHash<QString, QByteArray> g_symbolDict;

/*! constructs command line of htags(1) and executes it.
 *  \retval true success
 *  \retval false an error has occurred.
 */
bool Htags::execute(const QString &htmldir)
{
   static const QStringList inputSource = Config::getList("input-source");

   static bool quiet    = Config::getBool("quiet");
   static bool warnings = Config::getBool("warnings");

   static QByteArray htagsOptions  = "";       // Config::getString("htags-options");

   static QString projectName    = Config::getString("project-name");
   static QString projectVersion = Config::getString("project-version");

   QByteArray cwd = QDir::currentPath().toUtf8();

   if (inputSource.isEmpty()) {
      g_inputDir.setPath(cwd);

   } else if (inputSource.count() == 1) {
      g_inputDir.setPath(inputSource.first());

      if (! g_inputDir.exists())
         err("Unable to find directory %s, verify the value of the 'INPUT SOURCE' tag.\n", qPrintable(inputSource.first()) );

   } else {
      err("When 'USE HTAGS' are used, 'INPUT SOURCE' must specify a single directory\n");
      return false;
   }

   /*
    * Construct command line for htags(1).
    */
   QByteArray commandLine = " -g -s -a -n ";
   if (! quiet) {
      commandLine += "-v ";
   }

   if (warnings) {
      commandLine += "-w ";
   }

   if (! htagsOptions.isEmpty()) {
      commandLine += ' ';
      commandLine += htagsOptions;
   }

   if (! projectName.isEmpty()) {
      commandLine += "-t \"";
      commandLine += projectName;

      if (! projectVersion.isEmpty()) {
         commandLine += '-';
         commandLine += projectVersion;
      }
      commandLine += "\" ";
   }

   commandLine += " \"" + htmldir.toUtf8() + "\"";

   QString oldDir = QDir::currentPath();
   QDir::setCurrent(g_inputDir.absolutePath());
  
   portable_sysTimerStart();

   bool result = portable_system("htags", commandLine, false) == 0;
   portable_sysTimerStop();

   QDir::setCurrent(oldDir);

   return result;
}


/*! load filemap and make index.
 *  \param htmlDir of HTML directory generated by htags(1).
 *  \retval true success
 *  \retval false error
 */
bool Htags::loadFilemap(const QString &htmlDir)
{
   QString fileMapName = htmlDir + "/HTML/FILEMAP";
  
   QFileInfo fi(fileMapName);

   /*
    * Construct FILEMAP dictionary using QHash.
    *
    * In FILEMAP, URL includes 'html' suffix but we cut it off according
    * to the method of FileDef class.
    *
    * FILEMAP format:
    * <NAME>\t<HREF>.html\n
    * QDICT:
    * dict[<NAME>] = <HREF>
    */

   if (fi.exists() && fi.isReadable()) {
      QFile f(fileMapName);
     
      QByteArray line;     

      if (f.open(QIODevice::ReadOnly)) {

         while (! (line = f.readLine()).isEmpty()) {
           
            int sep = line.indexOf('\t');

            if (sep != -1) {
               QByteArray key   = line.left(sep).trimmed();
               QByteArray value = line.mid(sep + 1).trimmed();

               int ext = value.lastIndexOf('.');

               if (ext != -1) {
                  value = value.left(ext);   // strip extension
               }
              
               g_symbolDict.insert(key, value);               
            }
         }
         return true;

      } else {
         err("Unable to open file %s, error: %d\n", qPrintable(fileMapName), f.error());  
         
      }
   }

   return false;
}

/*! convert path name into the url in the hypertext generated by htags.
 *  \param path path name
 *  \returns URL NULL: not found.
 */
QByteArray Htags::path2URL(const QString &path)
{
   QByteArray url;
   QByteArray symName = path.toUtf8();
   QByteArray dir = g_inputDir.absolutePath().toUtf8();

   int dl = dir.length();

   if (symName.length() > dl + 1) {
      symName = symName.mid(dl + 1);
   }

   if (! symName.isEmpty()) {
      QByteArray result = g_symbolDict[symName];

      if (! result.isEmpty()) {
         url = "HTML/" + result;
      }
   }
   return url;
}

