package tree_sitter_candid_test

import (
	"testing"

	tree_sitter "github.com/tree-sitter/go-tree-sitter"
	tree_sitter_candid "github.com/tree-sitter/tree-sitter-candid/bindings/go"
)

func TestCanLoadGrammar(t *testing.T) {
	language := tree_sitter.NewLanguage(tree_sitter_candid.Language())
	if language == nil {
		t.Errorf("Error loading Candid grammar")
	}
}
