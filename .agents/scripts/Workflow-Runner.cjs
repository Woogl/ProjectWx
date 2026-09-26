// Copyright Woogle. All Rights Reserved.
// 웹에서 맡긴 AI 처리를 터미널 창에서 보이게 실행하고 결과를 작업 폴더에 남긴다: node Workflow-Runner.cjs <작업 폴더>
const fs=require('node:fs'),path=require('node:path');
const {labels,runProvider}=require('./Workflow-Providers.cjs');
const job=process.argv[2],file=name=>path.join(job,name);
function finish(result){const temp=file('result.json.tmp');fs.writeFileSync(temp,JSON.stringify(result));fs.renameSync(temp,file('result.json'));}
(async()=>{
  fs.writeFileSync(file('pid.txt'),String(process.pid));
  let summary='';
  try{
    const {provider,command,mode,title,repo}=JSON.parse(fs.readFileSync(file('job.json'),'utf8'));
    console.log(`Wx AI · ${title} · ${labels[provider]||provider} · ${mode==='plan'?'조사(읽기 전용, 30분 제한)':'작업(모든 명령 허용, 60분 제한)'}`);
    console.log(provider==='gemini'?'처리 중입니다. Gemini CLI는 끝난 뒤 결과만 표시합니다.\n':'진행 과정이 아래에 표시됩니다.\n');
    const value=await runProvider({provider,command,prompt:fs.readFileSync(file('prompt.txt'),'utf8'),repo,output:file('output.json'),schema:file('schema.json'),mode,visible:true});
    finish({ok:true,value});summary='처리를 마쳤습니다. '+(value?.summary||'');
  }catch(error){finish({ok:false,error:error.message});summary='처리하지 못했습니다. '+error.message;}
  const seconds=Number(process.env.WX_RUNNER_CLOSE_SECONDS||60);
  console.log(`\n${summary}\n결과는 웹 화면에 반영됩니다. 이 창은 ${seconds}초 뒤 닫힙니다(Enter를 누르면 바로 닫힘).`);
  const timer=setTimeout(()=>process.exit(0),seconds*1000);
  process.stdin.once('data',()=>{clearTimeout(timer);process.exit(0);});
  process.stdin.on('error',()=>{});
})();
