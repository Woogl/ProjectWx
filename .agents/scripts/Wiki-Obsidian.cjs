// Copyright Woogle. All Rights Reserved.
// 대시보드 Wiki 갱신에서 AI가 claude-obsidian 명령을 부르는 입구다: node Wiki-Obsidian.cjs <명령> [인자...]
// claude-obsidian은 Windows에서 vault를 쓰지 못해 명령을 WSL에서 실행한다. 드라이런과 적용이 같은 환경이어야 승인 해시가 맞으므로 읽기 명령도 여기로 부른다.
// 기본 /mnt/c는 파일 권한을 저장하지 못해 쓰기가 RESULT_DRIFT로 되돌려지므로, 저장소 드라이브를 metadata 옵션으로 따로 붙인다(WSL을 다시 시작하면 사라져 명령마다 확인한다).
const fs = require('node:fs');
const path = require('node:path');
const { spawnSync } = require('node:child_process');
const repo = path.resolve(__dirname, '../..');
const distro = 'Ubuntu';
const mounted = drive => `/mnt/wx-${drive.toLowerCase()}`;
function toWsl(file) {
  const match = /^([A-Za-z]):[\\/]*(.*)$/.exec(path.resolve(file));
  if (!match) throw Error('WSL로 넘길 수 없는 경로입니다: ' + file);
  return (mounted(match[1]) + '/' + match[2].replace(/\\/g, '/')).replace(/\/+$/, '');
}
function wslArgs(args, cwd = process.cwd(), root = repo) {
  const tree = path.join(root, 'Saved/Workflow/wiki-update-tree');
  const tag = fs.readFileSync(path.join(tree, 'Wiki/README.md'), 'utf8').match(/AgriciDaniel\/claude-obsidian#(v[0-9][\w.-]*)/)?.[1];
  if (!tag) throw Error('Wiki/README.md에서 claude-obsidian 태그를 찾지 못했습니다.');
  const cli = path.join(root, 'Saved/Workflow/claude-obsidian', tag, 'scripts/claude-obsidian.py');
  const drive = path.resolve(root)[0];
  const script = 'm=$1; src=$2; dir=$3; shift 3; mountpoint -q "$m" || { mkdir -p "$m" && mount -t drvfs "$src" "$m" -o metadata; } || exit 70; cd "$dir" && exec python3 -B "$@"';
  return ['-d', distro, '-u', 'root', '-e', 'sh', '-c', script, 'sh', mounted(drive), drive + ':\\', toWsl(cwd), toWsl(cli),
    ...args.map(arg => /^[A-Za-z]:[\\/]/.test(arg) ? toWsl(arg) : arg)];
}
if (require.main === module) {
  let result;
  try { result = spawnSync('wsl.exe', wslArgs(process.argv.slice(2)), { stdio: 'inherit', windowsHide: true }); }
  catch (error) { console.error(error.message); process.exit(2); }
  if (result.error) { console.error('wsl.exe를 실행하지 못했습니다: ' + result.error.message); process.exit(2); }
  process.exit(result.status ?? 1);
}
module.exports = { toWsl, wslArgs };
