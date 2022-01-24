//
// Created by Mohamed El Zarei on 08/01/2022.
//

#ifndef ZREGEXSTANDALONE_ANTLRNODEVISITOR_H
#define ZREGEXSTANDALONE_ANTLRNODEVISITOR_H

#include <memory>
#include <vector>

#include "fa/fa.h"
#include "fa/factory.h"
#include "parser/grammar/regexVisitor.h"
#include "spdlog/fmt/fmt.h"

namespace ZRegex {
  class AntlrNodeVisitor : public regexVisitor {
    antlrcpp::Any visitAlternation(regexParser::AlternationContext *context) override {
      // single element return it
      if (context->expression().size() == 1) {
        auto r = std::move(
            context->expression(0)->accept(this).as<std::unique_ptr<FiniteAutomaton>>());
        return r;
      }
      std::vector<std::unique_ptr<FiniteAutomaton>> automatons;
      for (const auto &expr : context->expression()) {
        auto expr_nfa = std::move(expr->accept(this).as<std::unique_ptr<FiniteAutomaton>>());
        automatons.push_back(std::move(expr_nfa));
      }

      return FAFactory::UnionAll(automatons);
    }

  public:
    antlrcpp::Any visitRegex(regexParser::RegexContext *context) override {
      return context->alternation()->accept(this);
    }
    antlrcpp::Any visitExpression(regexParser::ExpressionContext *context) override {
      // single element
      if (context->element().size() == 1) {
        auto z
            = std::move(context->element(0)->accept(this).as<std::unique_ptr<FiniteAutomaton>>());
        return z;
      }

      // concat all elements
      std::vector<std::unique_ptr<FiniteAutomaton>> automatons;
      for (const auto &expr : context->element()) {
        auto z = std::move(expr->accept(this).as<std::unique_ptr<FiniteAutomaton>>());
        automatons.push_back(std::move(z));
      }
      return ZRegex::FAFactory::ConcatAll(automatons);
    }

    antlrcpp::Any visitElement(regexParser::ElementContext *context) override {
      auto atm = std::move(context->atom()->accept(this).as<std::unique_ptr<FiniteAutomaton>>());
      if (!context->quantifier()) {
        return atm;
      }
      auto min = context->quantifier()->min;
      auto max = context->quantifier()->max;
      if (context->quantifier()->Plus()) {
        return ZRegex::FAFactory::RepeatMinimum(std::move(atm), 1);
      } else if (context->quantifier()->Asterisk()) {
        return ZRegex::FAFactory::KleeneStar(std::move(atm));
      } else if (context->quantifier()->Qmark()) {
        return ZRegex::FAFactory::Optional(std::move(atm));
      } else if (!context->quantifier()->Comma()) {  // no comma thus exact match
        throw std::runtime_error("RepeatExact Not implemented yet!");
      } else if (min && max) {  // min and max defined
        auto min_reps = std::stoi(min->getText());
        auto max_reps = std::stoi(max->getText());
        if (max_reps < min_reps) {
          throw std::runtime_error(
              fmt::format("Max {} must be less than or equal to Min {}.", max_reps, min_reps));
        }
        throw std::runtime_error("RepeatMinMax Not implemented yet!");
      } else if (min) {  // min only defined and comma is defined thus at least min
        auto min_reps = min->getText();
        return ZRegex::FAFactory::RepeatMinimum(std::move(atm), std::stoi(min_reps));
      }
      return atm;
    }

    antlrcpp::Any visitQuantifier(regexParser::QuantifierContext *context) override {
      return antlrcpp::Any();
    }
    antlrcpp::Any visitNumber(regexParser::NumberContext *context) override {
      return antlrcpp::Any();
    }
    antlrcpp::Any visitAtom(regexParser::AtomContext *context) override {
      if (context->Wildcard()) {
        auto c = ZRegex::FAFactory::AnyChar();
        return ZRegex::FAFactory::KleeneStar(std::move(c));
      }
      if (context->capture) {
        auto v = std::move(context->capture->accept(this).as<std::unique_ptr<FiniteAutomaton>>());
        return std::move(v);
      }

      if (context->characterClass()) {
        auto v = std::move(context->characterClass()->accept(this).as<std::unique_ptr<FiniteAutomaton>>());
        return std::move(v);
      }

      auto v = std::move(context->character()->accept(this).as<std::unique_ptr<FiniteAutomaton>>());
      return std::move(v);
    }

    antlrcpp::Any visitCharacter(regexParser::CharacterContext *context) override {
      std::unique_ptr<FiniteAutomaton> v;
      if (context->specialChar()) {
        v = std::move(context->specialChar()->accept(this).as<std::unique_ptr<FiniteAutomaton>>());
      } else {
        v = std::move(
            context->regularCharacter()->accept(this).as<std::unique_ptr<FiniteAutomaton>>());
      }
      return std::move(v);
    };

    antlrcpp::Any visitRegularCharacter(regexParser::RegularCharacterContext *context) override {
      return ZRegex::FAFactory::StringAutomaton(context->value->getText());
    }

    antlrcpp::Any visitSpecialChar(regexParser::SpecialCharContext *context) override {
      return ZRegex::FAFactory::StringAutomaton(context->value->getText());
    }

    antlrcpp::Any visitAnyChar(regexParser::AnyCharContext *context) override {
      return ZRegex::FAFactory::AnyChar();
    }
    antlrcpp::Any visitCcPositive(regexParser::CcPositiveContext *context) override {
      return antlrcpp::Any();
    }
    antlrcpp::Any visitCcNegative(regexParser::CcNegativeContext *context) override {
      return antlrcpp::Any();
    }
    antlrcpp::Any visitCcMixed(regexParser::CcMixedContext *context) override {
      return antlrcpp::Any();
    }
    antlrcpp::Any visitClassMember(regexParser::ClassMemberContext *context) override {
      return antlrcpp::Any();
    }
    antlrcpp::Any visitRange(regexParser::RangeContext *context) override {

      auto min = std::move(context->min->accept(this).as<std::unique_ptr<FiniteAutomaton>>());
      auto max = std::move(context->max->accept(this).as<std::unique_ptr<FiniteAutomaton>>());

      return ZRegex::FAFactory::CharRangeAutomaton(min->initial_state->tra);
    }
    antlrcpp::Any visitPredefinedClass(regexParser::PredefinedClassContext *context) override {
      return antlrcpp::Any();
    }
    antlrcpp::Any visitPredefinedClassName(
        regexParser::PredefinedClassNameContext *context) override {
      return antlrcpp::Any();
    }
  };
};  // namespace ZRegex

#endif  // ZREGEXSTANDALONE_ANTLRNODEVISITOR_H