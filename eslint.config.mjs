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
];


export default EslintConfig;
