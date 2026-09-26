// Copyright Woogle. All Rights Reserved.
const http = require('node:http');
const fs = require('node:fs');
const path = require('node:path');
const crypto = require('node:crypto');
const { execFile } = require('node:child_process');
const { labels } = require('./Wiki-AI-Providers.cjs');
const { createFeedbackService, runJob, runTerminalJob, openTerminal, openSession, readTasks } = require('./Workflow-TestFeedback.cjs');
const repo = path.resolve(__dirname, '../..');
const identity = crypto.createHash('sha256').update(repo.toLowerCase()).digest('hex');
const protocol = 4;
// Start-WikiAI.ps1이 같은 파일 목록으로 비교해 코드가 바뀐 서버만 다시 띄운다.
const revision = [__filename, ...['Wiki-AI-Providers.cjs','wiki-gemini-settings.json','Start-WikiAI.ps1','Workflow-TestFeedback.cjs','wiki-viewer/task-records.js','Workflow-Runner.cjs'].map(file=>path.join(__dirname,file))]
  .map(file => crypto.createHash('sha256').update(fs.readFileSync(file)).digest('hex')).join(':');
const execText=(file,args,options={})=>new Promise((resolve,reject)=>execFile(file,args,{windowsHide:true,timeout:10*60*1000,maxBuffer:8*1024*1024,...options},(error,stdout)=>error?reject(error):resolve(String(stdout).trim())));
const wikiSchema={type:'object',additionalProperties:false,required:['summary','evidence'],properties:{summary:{type:'string'},evidence:{type:'array',items:{type:'string'}}}};
// 대시보드의 Wiki 갱신: 고른 AI가 이 PC에서 Wiki/README.md 절차로 Wiki를 갱신해 main에 푸시한다.
// 사용자 작업 트리와 섞이지 않게 origin/main의 sparse 작업 트리에서 하고, 준비물(WSL, claude-obsidian)이 없으면 설치를 시작한다.
function createWikiUpdate({root,providers,commands,exec=execText,open=openTerminal,run=runTerminalJob,now=()=>new Date().toISOString()}){
  const tree=path.join(root,'Saved/Workflow/wiki-update-tree');
  let state={status:'idle'};
  const busy=()=>['preparing','running'].includes(state.status);
  const step=message=>{state={...state,message};};
  const git=(...args)=>exec('git',args,{cwd:root});
  // claude-obsidian은 Windows에서 vault를 쓰지 못해 쓰기 명령은 WSL에서 돈다. 배포판이 없으면 재부팅 대기가 아닌 한 보이는 안내 창에서 관리자 승인으로 설치를 시작한다.
  // WSL이 설치된 뒤에는 배포판이 없어도 `wsl -l -q`가 성공하고 빈 목록(UTF-16)을 돌려준다.
  async function wslState(){
    try{await exec('wsl.exe',['-e','python3','--version'],{timeout:3*60*1000});return 'ready';}catch{}
    let distros='';
    try{distros=await exec('wsl.exe',['-l','-q'],{encoding:'utf16le'});}catch{}
    if(distros.replace(/\0/g,'').trim())throw Error('WSL에서 python3를 실행하지 못했습니다. Ubuntu 창을 열어 첫 설정(Linux 사용자 만들기)을 마쳤는지 확인하세요.');
    try{await exec('reg.exe',['query','HKLM\\SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Component Based Servicing\\RebootPending']);return 'reboot';}catch{}
    open(root,'Wx · WSL 설치',['powershell.exe','-NoExit','-NoProfile','-Command',"Write-Host 'WSL(Ubuntu)을 설치합니다. 관리자 승인 창에서 [예]를 누르세요.'; try { Start-Process -Verb RunAs -FilePath wsl.exe -ArgumentList '--install','-d','Ubuntu' -Wait -ErrorAction Stop; Write-Host '설치 창이 닫혔습니다. 재부팅 안내가 있었다면 재부팅하고, Ubuntu 창에서 Linux 사용자를 만든 뒤 대시보드에서 Wiki 갱신을 다시 누르세요.' } catch { Write-Host ('설치를 시작하지 못했습니다: ' + $_.Exception.Message) }"]);
    return 'install';
  }
  const setupMessages={reboot:'Windows를 다시 시작해야 WSL 설치가 끝납니다. 재부팅한 뒤 Wiki 갱신을 다시 누르세요.',install:'WSL(Ubuntu) 설치 안내 창을 열었습니다. 창의 안내대로 관리자 승인을 허용하고, 재부팅 안내가 나오면 재부팅한 뒤, Ubuntu 창에서 Linux 사용자를 만들고 Wiki 갱신을 다시 누르세요.'};
  async function prepareTree(){
    await git('fetch','--quiet','origin','main');
    if(!fs.existsSync(path.join(tree,'.git'))){
      if(fs.existsSync(tree))throw Error(`${tree}가 Git 작업 트리가 아닙니다. 이 폴더를 지운 뒤 다시 누르세요.`);
      await git('worktree','prune');
      await git('worktree','add','--quiet','--no-checkout','--detach',tree,'origin/main');
      await git('-C',tree,'sparse-checkout','set','--no-cone','/Wiki/','/Docs/**/*.md','/.agents/workflow/tasks/','/AGENTS.md','/.gitattributes');
    }
    await git('-C',tree,'reset','--quiet','--hard','origin/main');
    await git('-C',tree,'clean','-q','-fdx');
  }
  // 버전은 Wiki/README.md의 설정 스크립트에 적힌 태그를 따른다. 저장소 밖(Saved)에 순정 코드를 받아 모든 AI가 같은 것을 쓴다.
  async function plugin(){
    const tag=fs.readFileSync(path.join(tree,'Wiki/README.md'),'utf8').match(/AgriciDaniel\/claude-obsidian#(v[0-9][\w.-]*)/)?.[1];
    if(!tag)throw Error('Wiki/README.md의 설정 스크립트에서 claude-obsidian 태그를 찾지 못했습니다.');
    const dir=path.join(root,'Saved/Workflow/claude-obsidian',tag);
    if(!fs.existsSync(path.join(dir,'scripts/claude-obsidian.py'))){
      step(`claude-obsidian ${tag}을 받는 중입니다.`);
      const download=dir+'.download';
      fs.rmSync(download,{recursive:true,force:true});
      await exec('git',['-c','core.autocrlf=false','clone','--quiet','--depth','1','--branch',tag,'https://github.com/AgriciDaniel/claude-obsidian',download]);
      fs.rmSync(dir,{recursive:true,force:true});fs.renameSync(download,dir);
    }
    return {tag,dir};
  }
  async function work(provider){
    try{
      step('WSL을 확인하는 중입니다.');
      const setup=await wslState();
      if(setup!=='ready'){state={...state,status:'setup',message:setupMessages[setup],at:now()};return;}
      step('Wiki 작업 트리를 origin/main으로 맞추는 중입니다.');
      await prepareTree();
      const {tag,dir}=await plugin();
      const wsl=file=>exec('wsl.exe',['-e','wslpath','-a',file]);
      const info={worktree:tree,worktreeWsl:await wsl(tree),claudeObsidian:{tag,path:dir,wslPath:await wsl(dir)}};
      state={...state,status:'running',message:'AI가 작업하는 중입니다. 진행 과정은 터미널 창에 보입니다.'};
      const prompt=`한국어로 작업하세요. 지금 폴더는 Wiki 즉시 갱신용 작업 트리입니다(origin/main을 받은 sparse 사본). Wiki/README.md의 정기 갱신 절차와 서술 규칙을 따라 지금 Wiki를 갱신하세요. 이 PC(Windows)에서 도는 경우의 규칙도 그 문서에 있습니다.
이 PC 정보(JSON): ${JSON.stringify(info)}
관리자 정책과 CLI 설정을 바꾸지 마세요. 이 작업 트리 밖의 파일은 고치지 마세요.
summary에는 수집한 원자료, 바뀐 노트, lint 결과, 커밋 해시, 실패하거나 건너뛴 단계를 짧게 적고, evidence에는 실제로 실행한 명령과 결과를 적으세요.`;
      const value=await run({root,repo:tree,command:commands[provider],provider,mode:'work',title:'Wiki 갱신',id:'wiki-update-'+Date.now(),prompt,schema:wikiSchema});
      if(typeof value?.summary!=='string'||!value.summary.trim()||!Array.isArray(value.evidence))throw Error('AI 결과 형식이 올바르지 않습니다.');
      state={...state,status:'complete',message:'',summary:value.summary,evidence:value.evidence,at:now()};
    }catch(error){state={...state,status:'failed',message:'',error:error.message,at:now()};}
  }
  function act(body){
    if(body?.action==='status')return state;
    if(body?.action!=='start')throw Error('Wiki 갱신 요청 형식 오류');
    if(!providers.includes(body.provider))throw Error('선택한 AI를 사용할 수 없습니다. 설치 후 OpenWorkflow.bat을 다시 실행하세요.');
    if(busy())throw Error('Wiki 갱신이 이미 진행 중입니다.');
    state={status:'preparing',provider:body.provider,startedAt:now(),message:'준비하는 중입니다.'};
    work(body.provider);
    return state;
  }
  return {act,isBusy:busy};
}
function createServer({token,port=18743,configuration='',testFeedback=null,wikiUpdate=null}) {
  return http.createServer(async(request,response)=>{
    response.setHeader('Cache-Control','no-store');response.setHeader('Content-Type','application/json; charset=utf-8');
    const send=(status,body)=>{response.writeHead(status);response.end(JSON.stringify(body));};
    if(request.headers.host!==`127.0.0.1:${port}`)return send(403,{error:'접근할 수 없습니다.'});
    if(request.method==='GET'&&request.url==='/health')return send(200,{identity,protocol,revision,busy:!!testFeedback?.isBusy()||!!wikiUpdate?.isBusy(),configuration});
    if(request.headers.origin!=='null')return send(403,{error:'OpenWorkflow 파일에서 요청하세요.'});
    response.setHeader('Access-Control-Allow-Origin','null');response.setHeader('Access-Control-Allow-Private-Network','true');
    if(!['/test-feedback','/wiki-update'].includes(request.url)||!['POST','OPTIONS'].includes(request.method))return send(404,{error:'지원하지 않는 요청입니다.'});
    if(request.method==='OPTIONS'){response.setHeader('Access-Control-Allow-Methods','POST');response.setHeader('Access-Control-Allow-Headers','Content-Type, X-Wx-Token');return send(204,{});}
    if(request.headers['x-wx-token']!==token)return send(403,{error:'OpenWorkflow.bat을 다시 실행하세요.'});
    if(!request.headers['content-type']?.startsWith('application/json'))return send(400,{error:'JSON 입력이 필요합니다.'});
    try{
      const chunks=[];let size=0;
      for await(const chunk of request){size+=chunk.length;if(size>2*1024*1024)return send(413,{error:'요청이 2MB를 초과했습니다.'});chunks.push(chunk);}
      let body;try{body=JSON.parse(Buffer.concat(chunks).toString('utf8'));}catch{return send(400,{error:'입력을 읽지 못했습니다.'});}
      if(request.url==='/wiki-update'){if(!wikiUpdate)throw Error('Wiki 갱신 연결이 없습니다. OpenWorkflow.bat을 다시 실행하세요.');return send(200,await wikiUpdate.act(body));}
      if(!testFeedback)throw Error('테스트 결과 연결이 없습니다. OpenWorkflow.bat을 다시 실행하세요.');
      return send(200,testFeedback.act(body));
    }catch(error){if(!response.destroyed&&!response.headersSent)send(400,{error:error.message});}
  });
}
// 작업 현황을 터미널에 출력한다: node .agents/scripts/Wiki-AI.cjs --tasks
if(require.main===module&&process.argv[2]==='--tasks'){
  const tasks=readTasks(repo);
  for(const state of ['확인 대기','진행 중','완료','']){
    const group=tasks.filter(task=>task.state===state);
    console.log(`\n${state||'상태 없음(리뷰·참고)'} · ${group.length}`);
    for(const task of group)console.log(`- ${task.title} | ${task.summary||'-'} | ${task.next||'-'} | ${task.path}`);
  }
}else if(require.main===module){
  const token=crypto.randomBytes(32).toString('hex');
  const config=JSON.parse(fs.readFileSync(process.argv[2],'utf8'));
  const providers=Object.keys(labels).filter(id=>config[id]?.file && fs.existsSync(config[id].file));
  const configuration=crypto.createHash('sha256').update(JSON.stringify(config)).digest('hex');
  const testFeedback=createFeedbackService({root:repo,providers,
    run:(request,onSpawn)=>runJob({root:repo,command:config[request.provider||'codex'],request,onSpawn}),
    open:(taskPath,provider,title)=>openSession({root:repo,command:config[provider],provider,taskPath,title})});
  const server=createServer({token,configuration,testFeedback,wikiUpdate:createWikiUpdate({root:repo,providers,commands:config})});
  server.on('error',error=>{console.error(error.message);process.exit(1);});
  server.listen(18743,'127.0.0.1',()=>{
    fs.mkdirSync(path.join(repo,'Saved/Workflow'),{recursive:true});
    fs.writeFileSync(path.join(repo,'Saved/Workflow/ai-connection.json'),JSON.stringify({token,url:'http://127.0.0.1:18743'}));
  });
}
module.exports={createServer,createWikiUpdate};
