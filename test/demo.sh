#!/bin/sh
#
# $Id: $
#
# Module:  demo -- description
# Created: 12-MAY-2010 01:44
# Author:  tmr
#

../xslfo2pdf < test.fo > test.pdf
../xslfo2svg < test.fo > test.svg
../xslfo2ps < test.fo > test.ps

# vim: fdm=syntax:fdn=3:tw=74:ts=2:syn=sh
