#! /usr/bin/env python
# -*- coding: utf-8 -*-

import fi
import unittest

tzer = fi.tokenize(u"""Tämä on kappale. Eipä ole kovin 2 nen, mutta tarkoitus on näyttää miten sanastaja 
toimii useiden-erilaisten sanaryppäiden kimpussa.
Pitääpä vielä "tarkistaa" sanat jotka 'lainausmerkeissä.""",
                   valid_chars=u"'")
for w in tzer:
    print w

#koe = fi.TestTokenizeFI()
#koe.test_tokenize_fi()

