import CommonConfig from "eslint-config-yscope/CommonConfig.mjs";
import StylisticConfigArray from "eslint-config-yscope/StylisticConfigArray.mjs";
import TsConfigArray from "eslint-config-yscope/TsConfigArray.mjs";
import TsdocPlugin from "eslint-plugin-tsdoc";


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
    {
        files: [
            "**/*.ts",
            "**/*.tsx",
        ],
        plugins: {
            tsdoc: TsdocPlugin,
        },
        rules: {
            "tsdoc/syntax": "warn",
        },
    },
    ...StylisticConfigArray,
];


export default EslintConfig;
