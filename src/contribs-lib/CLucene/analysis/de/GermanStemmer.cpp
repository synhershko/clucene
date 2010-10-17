/*------------------------------------------------------------------------------
* Copyright (C) 2003-2010 Ben van Klinken and the CLucene Team
* 
* Distributable under the terms of either the Apache License (Version 2.0) or 
* the GNU Lesser General Public License, as specified in the COPYING file.
------------------------------------------------------------------------------*/
#include "CLucene/_ApiHeader.h"
#include "CLucene/util/StringBuffer.h"
#include "GermanStemmer.h"

CL_NS_USE(util)
CL_NS_USE2(analysis,de)

    GermanStemmer::GermanStemmer() :
      sb() {
    }

    TCHAR* GermanStemmer::stem(const TCHAR* term, size_t length) {
      if (length < 0) {
        length = _tcslen(term);
      }

      // Reset the StringBuffer.
      sb.clear();
      sb.append(term, length);

      if (!isStemmable(sb.getBuffer(), sb.length()))
        return sb.giveBuffer();

      // Stemming starts here...
      substitute(sb);
      strip(sb);
      optimize(sb);
      resubstitute(sb);
      removeParticleDenotion(sb);

      return sb.giveBuffer();
    }

    bool GermanStemmer::isStemmable(const TCHAR* term, size_t length) const {
      if (length < 0) {
        length = _tcslen(term);
      }
      for (size_t c = 0; c < length; c++) {
        if (_istalpha(term[c]) == 0)
          return false;
      }
      return true;
    }

    void GermanStemmer::strip(StringBuffer& buffer)
    {
      bool doMore = true;
      while ( doMore && buffer.length() > 3 ) {
        if ( ( buffer.length() + substCount > 5 ) &&
          buffer.substringEquals( buffer.length() - 2, buffer.length(), _T("nd"), 2 ) )
        {
          buffer.deleteChars( buffer.length() - 2, buffer.length() );
        }
        else if ( ( buffer.length() + substCount > 4 ) &&
          buffer.substringEquals( buffer.length() - 2, buffer.length(), _T("em"), 2 ) ) {
            buffer.deleteChars( buffer.length() - 2, buffer.length() );
        }
        else if ( ( buffer.length() + substCount > 4 ) &&
          buffer.substringEquals( buffer.length() - 2, buffer.length(), _T("er"), 2 ) ) {
            buffer.deleteChars( buffer.length() - 2, buffer.length() );
        }
        else if ( buffer.charAt( buffer.length() - 1 ) == _T('e') ) {
          buffer.deleteCharAt( buffer.length() - 1 );
        }
        else if ( buffer.charAt( buffer.length() - 1 ) == _T('s') ) {
          buffer.deleteCharAt( buffer.length() - 1 );
        }
        else if ( buffer.charAt( buffer.length() - 1 ) == _T('n') ) {
          buffer.deleteCharAt( buffer.length() - 1 );
        }
        // "t" occurs only as suffix of verbs.
        else if ( buffer.charAt( buffer.length() - 1 ) == _T('t') ) {
          buffer.deleteCharAt( buffer.length() - 1 );
        }
        else {
          doMore = false;
        }
      }
    }

    void GermanStemmer::optimize(StringBuffer& buffer) {
      // Additional step for female plurals of professions and inhabitants.
      if ( buffer.length() > 5 && buffer.substringEquals( buffer.length() - 5, buffer.length(), _T("erin*"), 5 ) ) {
        buffer.deleteCharAt( buffer.length() -1 );
        strip( buffer );
      }
      // Additional step for irregular plural nouns like "Matrizen -> Matrix".
      if ( buffer.charAt( buffer.length() - 1 ) == ( _T('z') ) ) {
        buffer.setCharAt( buffer.length() - 1, _T('x') );
      }
    }

    void GermanStemmer::removeParticleDenotion(StringBuffer& buffer) {
      if ( buffer.length() > 4 ) {
        for ( size_t c = 0; c < buffer.length() - 3; c++ ) {
          if ( buffer.substringEquals( c, c + 4, _T("gege"), 4 ) ) {
            buffer.deleteChars( c, c + 2 );
            return;
          }
        }
      }
    }

    void GermanStemmer::substitute(StringBuffer& buffer) {
      substCount = 0;

      for ( size_t c = 0; c < buffer.length(); c++ ) {
        // Replace the second char of a pair of the equal characters with an asterisk
        if ( c > 0 && buffer.charAt( c ) == buffer.charAt ( c - 1 )  ) {
          buffer.setCharAt( c, _T('*') );
        }
        // Substitute Umlauts.
        else if ( buffer.charAt( c ) == _T('ä') ) {
          buffer.setCharAt( c, _T('a') );
        }
        else if ( buffer.charAt( c ) == _T('ö') ) {
          buffer.setCharAt( c, _T('o') );
        }
        else if ( buffer.charAt( c ) == _T('ü') ) {
          buffer.setCharAt( c, _T('u') );
        }
        // Fix bug so that 'ß' at the end of a word is replaced.
        else if ( buffer.charAt( c ) == _T('ß') ) {
            buffer.setCharAt( c, _T('s') );
            buffer.insert( c + 1, _T('s') );
            substCount++;
        }
        // Take care that at least one character is left left side from the current one
        if ( c < buffer.length() - 1 ) {
          // Masking several common character combinations with an token
          if ( ( c < buffer.length() - 2 ) && buffer.charAt( c ) == _T('s') &&
            buffer.charAt( c + 1 ) == _T('c') && buffer.charAt( c + 2 ) == _T('h') )
          {
            buffer.setCharAt( c, _T('$') );
            buffer.deleteChars( c + 1, c + 3 );
            substCount =+ 2;
          }
          else if ( buffer.charAt( c ) == _T('c') && buffer.charAt( c + 1 ) == _T('h') ) {
            buffer.setCharAt( c, _T('§') );
            buffer.deleteCharAt( c + 1 );
            substCount++;
          }
          else if ( buffer.charAt( c ) == _T('e') && buffer.charAt( c + 1 ) == _T('i') ) {
            buffer.setCharAt( c, _T('%') );
            buffer.deleteCharAt( c + 1 );
            substCount++;
          }
          else if ( buffer.charAt( c ) == _T('i') && buffer.charAt( c + 1 ) == _T('e') ) {
            buffer.setCharAt( c, _T('&') );
            buffer.deleteCharAt( c + 1 );
            substCount++;
          }
          else if ( buffer.charAt( c ) == _T('i') && buffer.charAt( c + 1 ) == _T('g') ) {
            buffer.setCharAt( c, _T('#') );
            buffer.deleteCharAt( c + 1 );
            substCount++;
          }
          else if ( buffer.charAt( c ) == _T('s') && buffer.charAt( c + 1 ) == _T('t') ) {
            buffer.setCharAt( c, _T('!') );
            buffer.deleteCharAt( c + 1 );
            substCount++;
          }
        }
      }
    }

    void GermanStemmer::resubstitute(StringBuffer& buffer) {
      for ( size_t c = 0; c < buffer.length(); c++ ) {
        if ( buffer.charAt( c ) == _T('*') ) {
          TCHAR x = buffer.charAt( c - 1 );
          buffer.setCharAt( c, x );
        }
        else if ( buffer.charAt( c ) == _T('$') ) {
          buffer.setCharAt( c, 's' );
          TCHAR ch[] = { _T('c'), _T('h')};
          buffer.insert( c + 1, ch );
        }
        else if ( buffer.charAt( c ) == _T('§') ) {
          buffer.setCharAt( c, _T('c') );
          buffer.insert( c + 1, _T('h') );
        }
        else if ( buffer.charAt( c ) == _T('%') ) {
          buffer.setCharAt( c, _T('e') );
          buffer.insert( c + 1, _T('i') );
        }
        else if ( buffer.charAt( c ) == _T('&') ) {
          buffer.setCharAt( c, _T('i') );
          buffer.insert( c + 1, _T('e') );
        }
        else if ( buffer.charAt( c ) == _T('#') ) {
          buffer.setCharAt( c, _T('i') );
          buffer.insert( c + 1, _T('g') );
        }
        else if ( buffer.charAt( c ) == _T('!') ) {
          buffer.setCharAt( c, _T('s') );
          buffer.insert( c + 1, _T('t') );
        }
      }
    }
