import CommonConfig from "eslint-config-yscope/CommonConfig.mjs";
import StylisticConfigArray from "eslint-config-yscope/StylisticConfigArray.mjs";
import TsConfigArray from "eslint-config-yscope/TsConfigArray.mjs";
import TsdocPlugin from "eslint-plugin-tsdoc";


const EslintConfig = [
    {
        ignores: [
            "build/",
            "dist/",
            "docs/",
            "node_modules/",
            "test/",
        ],
    },
    {
        files: [
            "src/**/*.ts",
            "src/**/*.tsx",
        ],
        plugins: {
            tsdoc: TsdocPlugin,
        },
        rules: {
            "tsdoc/syntax": "error",
        },
    },
    CommonConfig,
    ...TsConfigArray,
    ...StylisticConfigArray,
];


export default EslintConfig;
