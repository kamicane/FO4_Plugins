#!/usr/bin/env node
import fs from "node:fs"
import path from "node:path"

const presetsPath = path.join(process.cwd(), "CMakePresets.json")
const vscodeDir = path.join(process.cwd(), ".vscode")
const tasksPath = path.join(vscodeDir, "tasks.json")

const presets = JSON.parse(fs.readFileSync(presetsPath, "utf8"))
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
	const projectDir = "${workspaceFolder}/Projects/" + preset.name

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
		label: `[${id}] Deploy`,
		type: "process",
		hide: false,
		command: "node",
		args: ["deploy-project.mjs", projectDir, "${env:USERPROFILE}/MO2Data/Fallout4/mods/" + displayName],
		options: { cwd: "${workspaceFolder}" },
		presentation: commonPresentation,
		problemMatcher: []
	})

	// Compile Papyrus task
	tasks.push({
		label: `[${id}] Compile Papyrus`,
		type: "process",
		hide: false,
		command: "${env:USERPROFILE}/bin/FO4/Papyrus_Compiler_1.10.163_Patched/PapyrusCompiler.exe",
		args: [path.join(projectDir, "project.ppj")],
		options: { cwd: projectDir },
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
			`[${id}] Compile Papyrus`
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

if (!fs.existsSync(vscodeDir)) {
	fs.mkdirSync(vscodeDir)
}

fs.writeFileSync(tasksPath, JSON.stringify({ version: "2.0.0", tasks }, null, 2))
