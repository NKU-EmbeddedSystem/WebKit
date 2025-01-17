/*
 * Copyright (C) 2021 Apple Inc. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY APPLE INC. AND ITS CONTRIBUTORS ``AS IS''
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
 * THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL APPLE INC. OR ITS CONTRIBUTORS
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF
 * THE POSSIBILITY OF SUCH DAMAGE.
 */

#pragma once

#include "CalcOperator.h"
#include "CalculationCategory.h"

namespace WebCore {

class CSSCalcExpressionNode;
class CSSCalcSymbolTable;
class CSSParserToken;
class CSSParserTokenRange;

enum CSSValueID : uint16_t;

class CSSCalcExpressionNodeParser {
public:
    explicit CSSCalcExpressionNodeParser(CalculationCategory destinationCategory, const CSSCalcSymbolTable& symbolTable)
        : m_destinationCategory(destinationCategory)
        , m_symbolTable(symbolTable)
    {
    }

    RefPtr<CSSCalcExpressionNode> parseCalc(CSSParserTokenRange, CSSValueID function);
    
private:
    char operatorValue(const CSSParserToken&);

    bool parseValue(CSSParserTokenRange&, RefPtr<CSSCalcExpressionNode>&);
    bool parseValueTerm(CSSParserTokenRange&, int depth, RefPtr<CSSCalcExpressionNode>&);
    bool parseCalcFunction(CSSParserTokenRange&, CSSValueID, int depth, RefPtr<CSSCalcExpressionNode>&);
    bool parseCalcSum(CSSParserTokenRange&, int depth, RefPtr<CSSCalcExpressionNode>&);
    bool parseCalcProduct(CSSParserTokenRange&, int depth, RefPtr<CSSCalcExpressionNode>&);
    bool parseCalcValue(CSSParserTokenRange&, int depth, RefPtr<CSSCalcExpressionNode>&);

    CalculationCategory m_destinationCategory;
    const CSSCalcSymbolTable& m_symbolTable;
};

}
