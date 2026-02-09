import { defineConfig } from "eslint/config";
import noFloatingPromise from "eslint-plugin-no-floating-promise";
import globals from "globals";
import path from "node:path";
import { fileURLToPath } from "node:url";
import js from "@eslint/js";
import { FlatCompat } from "@eslint/eslintrc";

const __filename = fileURLToPath(import.meta.url);
const __dirname = path.dirname(__filename);
const compat = new FlatCompat({
    baseDirectory: __dirname,
    recommendedConfig: js.configs.recommended,
    allConfig: js.configs.all
});

export default defineConfig([{
    extends: compat.extends("eslint:recommended"),

    plugins: {
        "no-floating-promise": noFloatingPromise,
    },

    languageOptions: {
        globals: {
            ...globals.browser,
        },
    },

    rules: {
        "no-undef": "off",
        "no-unused-vars": "off",

        "max-len": ["error", {
            ignoreComments: true,
            ignoreUrls: true,
            ignorePattern: "^(goog\\.require|goog\\.provide)\\(.*\\);$",
        }],

        camelcase: ["error", {
            allow: ["^opt_"],
            properties: "never",
        }],

        "block-scoped-var": "error",
        "guard-for-in": "error",
        "new-parens": "error",
        "no-array-constructor": "error",
        "no-confusing-arrow": "error",
        "no-constant-binary-expression": "error",
        "no-constructor-return": "error",
        "no-duplicate-imports": "error",
        "no-eq-null": "error",
        "no-extend-native": "error",
        "no-invalid-this": "error",
        "no-lone-blocks": "error",
        "no-multi-assign": "error",
        "no-new-native-nonconstructor": "error",
        "no-octal-escape": "error",
        "no-promise-executor-return": "error",
        "no-return-assign": "error",
        "no-self-compare": "error",
        "no-sequences": "error",
        "no-template-curly-in-string": "error",
        "no-throw-literal": "error",
        "no-unmodified-loop-condition": "error",
        "no-unneeded-ternary": "error",
        "no-unreachable-loop": "error",
        "no-unused-private-class-members": "error",
        "no-useless-computed-key": "error",
        "no-useless-concat": "error",
        "no-useless-constructor": "error",
        "no-useless-rename": "error",
        "no-useless-return": "error",
        "no-var": "error",
        "prefer-const": "error",
        "prefer-promise-reject-errors": "error",
        "semi-style": "error",
        "semi": "error",
        "no-floating-promise/no-floating-promise": "error",
    },
}, {
    files: ["example_js_standalone_smart_card_client_library/src/es-module-exports.js"],

    languageOptions: {
        ecmaVersion: 5,
        sourceType: "module",
    },
}]);
