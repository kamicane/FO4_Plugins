#!/usr/bin/env node
import { readFileSync, writeFileSync, mkdirSync, existsSync } from "node:fs"
import { join } from "node:path"

const presetsPath = join(process.cwd(), "CMakePresets.json")
const vscodeDir = join(process.cwd(), ".vscode")
const tasksPath = join(vscodeDir, "tasks.json")

const presets = JSON.parse(readFileSync(presetsPath, "utf8"))
// const nonHiddenConfigurePresets = (presets.configurePresets ?? []).filter((p) => !p.hidden)
const configurePresets = (presets.configurePresets ?? []).filter((p) => !p.hidden && p.displayName)
const buildPresets = (presets.buildPresets ?? []).filter((p) => !p.hidden && p.displayName)

const commonPresentation = {
	echo: true,
	reveal: "always",
	focus: true,
	panel: "shared",
	showReuseMessage: true,
	clear: false
}

const tasks = []

tasks.push({
	type: "cmake",
	label: `[Default] Cmake Build`,
	command: "build",
	targets: ["all"],
	preset: "__defaultBuildPreset__",
	group: { kind: "build", isDefault: true },
	presentation: commonPresentation,
	problemMatcher: []
})

for (const preset of buildPresets) {
	const id = preset.name
	const displayName = preset.displayName

	// CMake build task
	tasks.push({
		type: "cmake",
		label: `[${id}] Build`,
		command: "build",
		targets: ["all"],
		preset: id,
		group: { kind: "build", isDefault: false },
		presentation: commonPresentation,
		problemMatcher: []
	})

	// Deploy task
	tasks.push({
		label: `[${preset.name}] Deploy`,
		type: "process",
		hide: false,
		command: "node",
		args: ["deploy-project.mjs", preset.name, "${env:USERPROFILE}/MO2Data/Fallout4/mods/" + preset.displayName],
		options: { cwd: "${workspaceFolder}" },
		presentation: commonPresentation,
		problemMatcher: []
	})

	// Compile Papyrus task
	tasks.push({
		label: `[${preset.name}] Compile Papyrus Scripts`,
		type: "process",
		hide: false,
		command: "${env:USERPROFILE}/bin/FO4/Papyrus_Compiler_1.10.163_Patched/PapyrusCompiler.exe",
		args: ["${workspaceFolder}/Projects/" + `${preset.name}/project.ppj`],
		options: { cwd: "${workspaceFolder}/Projects/" + preset.name },
		presentation: commonPresentation,
		problemMatcher: []
	})

	// Pack task
	tasks.push({
		label: `[${id}] Pack`,
		type: "process",
		hide: false,
		command: "bash",
		args: ["pack-project.sh", id],
		options: { cwd: "${workspaceFolder}" },
		presentation: commonPresentation,
		problemMatcher: []
	})

	// Meta task: build + compile papyrus + deploy
	tasks.push({
		label: `[${id}] Build + Compile Papyrus`,
		dependsOn: [
			`[${id}] Build`,
			`[${id}] Compile Papyrus Scripts`
		],
		dependsOrder: "sequence",
		problemMatcher: []
	})
}

for (const preset of configurePresets) {
	const id = preset.name
	const displayName = preset.displayName

	// CMake configure task
	tasks.push({
		type: "cmake",
		label: `[${id}] Configure`,
		command: "configure",
		preset: id,
		presentation: commonPresentation,
		problemMatcher: []
	})
}

if (!existsSync(vscodeDir)) {
	mkdirSync(vscodeDir)
}

writeFileSync(tasksPath, JSON.stringify({ version: "2.0.0", tasks }, null, 2))
