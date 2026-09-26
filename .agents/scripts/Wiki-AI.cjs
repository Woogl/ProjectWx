// Copyright Woogle. All Rights Reserved.
const http = require('node:http');
const fs = require('node:fs');
const path = require('node:path');
const crypto = require('node:crypto');
const { labels } = require('./Wiki-AI-Providers.cjs');
const { createFeedbackService, runJob, openSession, readTasks } = require('./Workflow-TestFeedback.cjs');
const repo = path.resolve(__dirname, '../..');
const identity = crypto.createHash('sha256').update(repo.toLowerCase()).digest('hex');
const protocol = 4;
// Start-WikiAI.ps1이 같은 파일 목록으로 비교해 코드가 바뀐 서버만 다시 띄운다.
const revision = [__filename, ...['Wiki-AI-Providers.cjs','wiki-gemini-settings.json','Start-WikiAI.ps1','Workflow-TestFeedback.cjs','wiki-viewer/task-records.js','Workflow-Runner.cjs'].map(file=>path.join(__dirname,file))]
  .map(file => crypto.createHash('sha256').update(fs.readFileSync(file)).digest('hex')).join(':');
// 대시보드의 Wiki 갱신: 정기 갱신 Routine을 지금 한 번 실행한다. 설정({trigger, token})은 Git 밖 파일에만 두고 요청마다 다시 읽는다.
function createWikiUpdate({file,post=fetch}){
  let firing=false;
  const routine=()=>{try{const value=JSON.parse(fs.readFileSync(file,'utf8'));return /^trig_[A-Za-z0-9]+$/.test(value?.trigger)&&typeof value.token==='string'&&value.token.trim()?value:null;}catch{return null;}};
  async function act(body){
    if(body?.action==='status')return {configured:!!routine()};
    if(body?.action!=='fire')throw Error('Wiki 갱신 요청 형식 오류');
    const target=routine();
    if(!target)throw Error('Saved/Wiki/wiki-routine.json에 Routine의 trigger와 token을 적으세요.');
    if(firing)throw Error('Wiki 갱신을 요청하는 중입니다.');
    firing=true;
    try{
      const response=await post(`https://api.anthropic.com/v1/claude_code/routines/${target.trigger}/fire`,{method:'POST',headers:{Authorization:'Bearer '+target.token.trim(),'anthropic-version':'2023-06-01','anthropic-beta':'experimental-cc-routine-2026-04-01','Content-Type':'application/json'},body:'{}'});
      const result=await response.json().catch(()=>null);
      if(!response.ok)throw Error(`Routine이 요청을 거절했습니다(HTTP ${response.status}). ${result?.error?.message||''}`.trim());
      return {sessionUrl:typeof result?.claude_code_session_url==='string'?result.claude_code_session_url:''};
    }finally{firing=false;}
  }
  return {act};
}
function createServer({token,port=18743,configuration='',testFeedback=null,wikiUpdate=null}) {
  return http.createServer(async(request,response)=>{
    response.setHeader('Cache-Control','no-store');response.setHeader('Content-Type','application/json; charset=utf-8');
    const send=(status,body)=>{response.writeHead(status);response.end(JSON.stringify(body));};
    if(request.headers.host!==`127.0.0.1:${port}`)return send(403,{error:'접근할 수 없습니다.'});
    if(request.method==='GET'&&request.url==='/health')return send(200,{identity,protocol,revision,busy:!!testFeedback?.isBusy(),configuration});
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
  const server=createServer({token,configuration,testFeedback,wikiUpdate:createWikiUpdate({file:path.join(repo,'Saved/Wiki/wiki-routine.json')})});
  server.on('error',error=>{console.error(error.message);process.exit(1);});
  server.listen(18743,'127.0.0.1',()=>{
    fs.mkdirSync(path.join(repo,'Saved/Wiki'),{recursive:true});
    fs.writeFileSync(path.join(repo,'Saved/Wiki/ai-connection.json'),JSON.stringify({token,url:'http://127.0.0.1:18743'}));
  });
}
module.exports={createServer,createWikiUpdate};
