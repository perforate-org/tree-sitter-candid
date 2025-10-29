/**
 * @file Candid grammar for tree-sitter
 * @author Yota Inomoto <yota@perforate.org>
 * @license Apache-2.0
 */

/// <reference types="tree-sitter-cli/dsl" />
// @ts-check

const HEX = /[0-9-A-Fa-f]/;
const NUM = /[0-9](_?[0-9])*/;
const HEX_NUM = /[0-9A-Fa-f](_?[0-9A-Fa-f])*/;

const ASCII = /[\x20-\x21\x23-\x5b\x5d-\x7e]/u; // '\20'..'\7e' except " or \

const UTF8_ENC = [
  /[\xc2-\xdf][\x80-\xbf]/,
  /[\xe0][\xa0-\xbf][\x80-\xbf]/,
  /[\xed][\x80-\x9f][\x80-\xbf]/,
  /[\xe1-\xec\xee-\xef][\x80-\xbf]{2}/,
  /[\xf0][\x90-\xbf][\x80-\xbf]{2}/,
  /[\xf4][\x80-\x8f][\x80-\xbf]{2}/,
  /[\xf1-\xf3][\x80-\xbf]{3}/,
];

module.exports = grammar({
  name: "candid",

  word: ($) => $.id,
  extras: ($) => [/\s/, $.comment],
  externals: ($) => [$._block_comment],

  rules: {
    // --- Type Structure ---
    // Reference: https://github.com/dfinity/candid/blob/master/spec/Candid.md#type-structure

    // Core Grammar

    prog: ($) => seq(sep($.def, ";"), optional($.actor)),
    def: ($) =>
      choice(
        seq("type", $.id, "=", $.datatype),
        seq("import", optional("service"), $.text),
      ),
    actor: ($) =>
      seq(
        "service",
        optional(field("name", $.id)),
        ":",
        optional(seq(field("type_parameters", $.tuptype), "->")),
        field("return_type", choice($.actortype, $.id)),
        optional(";"),
      ),

    actortype: ($) => seq("{", sep($.methtype, ";"), "}"),
    methtype: ($) =>
      seq(field("name", $.name), ":", field("type", choice($.functype, $.id))),
    functype: ($) =>
      seq(
        field("type_parameters", $.tuptype),
        "->",
        field("return_type", $.tuptype),
        repeat($.funcann),
      ),
    funcann: (_) => choice("oneway", "query", "composite_query"),
    tuptype: ($) => seq("(", sep($.argtype, ","), ")"),
    argtype: ($) => choice($.datatype, $._argtype_short),

    _record_fieldtype: ($) =>
      choice(
        seq(field("index", $.nat), ":", $.datatype),
        seq(field("hasharg", $.name), ":", $.datatype), // <hash(name)> : <datatype>
        $.datatype, // N : <datatype>  where N is either 0 or previous + 1
      ),
    _variant_fieldtype: ($) =>
      choice(
        seq(field("index", $.nat), ":", $.datatype),
        seq(field("hasharg", $.name), ":", $.datatype), // <hash(name)> : <datatype>
        $.nat, // <nat> : null
        $.name, // <name> : null
      ),

    datatype: ($) => choice($.id, $.primtype, $.comptype),
    comptype: ($) => choice($.constype, $.reftype),

    primtype: ($) =>
      choice(
        $.numtype,
        "bool",
        "text",
        "null",
        "reserved",
        "empty",
        "principal",
      ),

    numtype: (_) =>
      choice(
        "nat",
        "nat8",
        "nat16",
        "nat32",
        "nat64",
        "int",
        "int8",
        "int16",
        "int32",
        "int64",
        "float32",
        "float64",
      ),

    constype: ($) =>
      choice(
        seq("opt", $.datatype),
        seq("vec", $.datatype),
        seq(
          "record",
          "{",
          sep(alias($._record_fieldtype, $.fieldtype), ";"),
          "}",
        ),
        seq(
          "variant",
          "{",
          sep(alias($._variant_fieldtype, $.fieldtype), ";"),
          "}",
        ),
        $._constype_short,
      ),

    reftype: ($) =>
      choice(seq("func", $.functype), seq("service", $.actortype)),

    name: ($) => choice($.id, $.text),

    // Syntactic Shorthands

    _argtype_short: ($) => seq(field("name", $.name), ":", $.datatype),
    _constype_short: (_) => "blob",

    // Comments

    comment: ($) => choice($._line_comment, $._block_comment),
    _line_comment: (_) => token(seq("//", /[^\n]*/)),

    // Interfaces

    desc: ($) =>
      seq(sep($.def, ";"), optional(seq($.service, token.immediate(";")))),
    service: ($) =>
      seq(
        "service",
        optional(field("name", $.id)),
        ":",
        field("return_type", choice($.actortype, $.id)),
      ),

    // --- Values ---
    // Reference: https://github.com/dfinity/candid/blob/master/spec/Candid.md#values

    val: ($) => choice($.primval, $.consval, $.refval, seq("(", $.annval, ")")),
    annval: ($) => choice($.val, seq($.val, ":", $.datatype)),
    primval: ($) =>
      choice($.nat, $.int, $.float, $.text, $.bool_literal, $.null_literal),
    bool_literal: (_) => choice("true", "false"),
    null_literal: (_) => "null",
    consval: ($) =>
      choice(
        seq("opt", $.val),
        seq("vec", "{", sep($.annval, ";"), "}"),
        seq(
          "record",
          "{",
          sep(alias($._record_fieldval, $.fieldval), ";"),
          "}",
        ),
        seq("variant", "{", alias($._variant_fieldval, $.fieldval), "}"),
        $._consval_short,
      ),

    _record_fieldval: ($) =>
      choice(
        seq(field("index", $.nat), "=", $.annval),
        seq(field("hasharg", $.name), "=", $.annval), // <hash(name)> = <annval>
        $.annval, // N = <annval>  where N is either 0 or previous + 1
      ),
    _variant_fieldval: ($) =>
      choice(
        seq(field("index", $.nat), "=", $.annval),
        seq(field("hasharg", $.name), "=", $.annval), // <hash(name)> = <annval>
        $.nat, // <nat> = null
        $.name, // <name> = null
      ),

    refval: ($) =>
      choice(
        seq("service", $.text), // canister URI
        seq("func", $.text, ".", $.name), // canister URI and message name
        seq("principal", $.text), // principal URI
      ),
    arg: ($) => seq("(", sep($.annval, ","), ")"),

    letter: (_) => /[A-Za-z]/,
    digit: (_) => /[0-9]/,
    id: (_) => /[A-Za-z_][A-Za-z0-9_]*/,

    nat: (_) => choice(NUM, token(seq("0x", HEX_NUM))),
    int: (_) => signedToken(NUM),
    float: (_) =>
      choice(
        signedToken(NUM, ".", optional(NUM)),
        signedToken(
          NUM,
          optional(seq(".", optional(NUM))),
          choice("e", "E"),
          optional(choice("+", "-")),
          NUM,
        ),
        signedToken("0x", HEX_NUM, ".", optional(HEX_NUM)),
        signedToken(
          "0x",
          HEX_NUM,
          optional(seq(".", optional(HEX_NUM))),
          choice("p", "P"),
          optional(choice("+", "-")),
          NUM,
        ),
      ),

    text: ($) => seq('"', repeat($._char), '"'),

    // Unicode scalar value (i.e., a codepoint that is not a surrogate part)
    _char: ($) =>
      choice(
        $._utf8,
        token(seq("\\", HEX, HEX)), // ASCII code
        token(seq("\\", /[nrt\\"']/)), // Escape
        token(seq("\\u{", HEX_NUM, "}")), // UTF-8 code
      ),
    _utf8: (_) => choice(ASCII, choice(...UTF8_ENC)),

    // Syntactic Shorthands

    _consval_short: ($) => seq("blob", $.text),
  },
});

/**
 * Creates a rule to match zero or more of the rules separated by the given separator
 *
 * @param {RuleOrLiteral} rule
 * @param {RuleOrLiteral} sep
 *
 * @returns {SeqRule}
 */
function sep(rule, sep) {
  return seq(repeat(seq(rule, token.immediate(sep))), optional(rule));
}

/**
 * Creates a rule to match a signed numeric token.
 *
 * @param {...RuleOrLiteral} parts
 *
 * @returns {TokenRule}
 */
function signedToken(...parts) {
  return token(seq(optional(choice("+", "-")), ...parts));
}
