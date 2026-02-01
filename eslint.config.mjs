import CommonConfig from "eslint-config-yscope/CommonConfig.mjs";
import StylisticConfigArray from "eslint-config-yscope/StylisticConfigArray.mjs";
import TsConfigArray, {createTsConfigOverride} from "eslint-config-yscope/TsConfigArray.mjs";


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
    createTsConfigOverride(
        ["test/**/*.ts"],
        "tsconfig.json"
    ),
    createTsConfigOverride(
        ["vitest.config.ts"],
        "tsconfig.json"
    ),
    ...StylisticConfigArray,
];

export default EslintConfig;
