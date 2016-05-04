#! /usr/bin/env python
# -*- coding: utf-8 -*-

##
# Copyright (C) 2015  Daniel Vrátil <dvratil@redhat.com>
#
# This library is free software; you can redistribute it and/or
# modify it under the terms of the GNU Lesser General Public
# License as published by the Free Software Foundation; either
# version 2.1 of the License, or (at your option) any later version.
#
# This library is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
# Lesser General Public License for more details.
#
# You should have received a copy of the GNU Lesser General Public
# License along with this library; if not, write to the Free Software
# Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
##


import os, sys, glob, operator
from grantlee_strings_extractor import TranslationOutputter

class KI18nExtractStrings(TranslationOutputter):
    def createOutput(self, template_filename, context_strings, outputfile):
        for context_string in context_strings:
            outputfile.write("// i18n: file: %s\n" % template_filename)
            if context_string.context:
                if not context_string.plural:
                    outputfile.write("i18nc(\"" + context_string.context + "\", \"" + context_string._string + "\");\n")
                else:
                    outputfile.write("i18ncp(\"" + context_string.context + "\", \"" + context_string._string + "\", \"" + context_string.plural + "\", 1);\n")
            else:
                if context_string.plural:
                    outputfile.write("i18np(\"" + context_string._string + "\", \"" + context_string.plural + "\", 1);\n")
                else:
                    outputfile.write("i18n(\"" + context_string._string + "\");\n")



if __name__ == "__main__":
    ex = KI18nExtractStrings()

    outputfile = sys.stdout

    files = reduce(operator.add, map(glob.glob, sys.argv[1:]))

    for filename in files:
        f = open(filename, "r")
        ex.translate(f, outputfile)

    outputfile.write("\n")
