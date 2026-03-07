#!/usr/bin/env node
import fs from 'node:fs'
import path from 'node:path'

const args = process.argv.slice(2)
if (args.length !== 2) {
	console.error(`Wrong arguments`)
	process.exit(2)
}

const SRC_DIR = args[0]
const TARGET_DIR = args[1]

console.log(`Linking ${SRC_DIR} to ${TARGET_DIR}`)

if (fs.existsSync(TARGET_DIR)) {
	fs.unlinkSync(TARGET_DIR)
}

fs.symlinkSync(SRC_DIR, TARGET_DIR, 'dir')

console.log('Deploy complete.')
process.exit(0)
