import CommonConfig from "eslint-config-yscope/CommonConfig.mjs";
import StylisticConfigArray from "eslint-config-yscope/StylisticConfigArray.mjs";
import TsConfigArray from "eslint-config-yscope/TsConfigArray.mjs";


const EslintConfig = [
    {
        ignores: [
            "build/",
            "dist/",
            "node_modules/",
        ],
    },
    CommonConfig,
    ...TsConfigArray,
    ...StylisticConfigArray,
    {
        files: ["test/**/*.ts"],
        rules: {
            "@stylistic/array-element-newline": "off",
            "dot-notation": "off",
            "jsdoc/require-description": "off",
            "jsdoc/require-returns": "off",
            "max-lines-per-function": "off",
            "max-statements": "off",
            "no-magic-numbers": "off",
        },
    },
];


export default EslintConfig;
