// Copyright Woogle. All Rights Reserved.
const fs=require('node:fs'),path=require('node:path'),vm=require('node:vm'),assert=require('node:assert/strict');
const elements=new Map(),storage=new Map();
class Element{
  constructor(tag){this.tagName=tag.toUpperCase();this.children=[];this.attributes={};this.value='';}
  set id(value){this._id=value;elements.set(value,this);}get id(){return this._id;}
  append(...nodes){for(const node of nodes){node.parentElement=this;this.children.push(node);}}
  replaceChildren(...nodes){this.children=[];this.append(...nodes);}
  setAttribute(name,value){this.attributes[name]=value;}getAttribute(name){return this.attributes[name];}
  querySelectorAll(selector){return descendants(this).filter(node=>selector==='textarea'?node.tagName==='TEXTAREA':node.tagName==='INPUT'&&node.type==='radio');}
}
function descendants(node){return [node,...node.children.flatMap(descendants)];}
function byId(id){if(!elements.has(id)){const node=new Element('div');node.id=id;}return elements.get(id);}
function el(tag,text,className){const node=new Element(tag);node.textContent=text;node.className=className;return node;}
const item={title:'저장·복원',path:'.agents/workflow/tasks/example.md',next:'재개 후 좌표 확인'};
let providers=[{id:'codex',label:'Codex'},{id:'claude',label:'Claude Code'},{id:'gemini',label:'Gemini CLI'}];
let revision=0,latest=null,starts=0,loss=false,refuse=false,savedRequest,sequence=0,recordsRendered=0;
const state=()=>({taskPath:item.path,providers,revision,latest,taskHash:'task-'+revision,codeVersion:'code'});
const api=async(route,body)=>{
  assert.equal(route,'/test-feedback');
  body=JSON.parse(JSON.stringify(body));
  if(body.action==='read')return {...state(),scope:item.next};
  if(body.action==='list')return {records:{[item.path]:state()},providers,indexText:'새 작업 목차'};
  if(refuse){const error=Error('다른 AI 작업 중');error.responded=true;throw error;}
  if(!savedRequest||savedRequest.operationId!==body.operationId){
    assert.equal(body.expectedRevision,revision);starts++;revision++;savedRequest=structuredClone(body);
    latest={...body,status:'running',at:'2026-09-25',report:null,error:''};
  }else assert.deepEqual(body,savedRequest,'unknown response retries must use the original request');
  if(loss){loss=false;throw Error('연결 끊김');}
  return state();
};
const context=vm.createContext({document:{},$:byId,el,workflowKey:'test',data:{ai:{},documents:[{path:'.agents/workflow/tasks/index.md',text:'old'}]},location:{hash:''},fetch:()=>{},
  localStorage:{getItem:k=>storage.get(k)||null,setItem:(k,v)=>storage.set(k,v),removeItem:k=>storage.delete(k)},
  workflowRequest:api,workflowButton:(label,click)=>{const node=el('button',label);node.onclick=click;return node;},requestId:()=>String(++sequence),renderTaskRecords:()=>{recordsRendered++;}
});
vm.runInContext(fs.readFileSync(path.join(__dirname,'wiki-viewer/test-feedback.js'),'utf8'),context);
const run=code=>vm.runInContext(code,context);context.item=item;
const input=label=>descendants(byId('test-feedback-panel')).find(node=>node.attributes['aria-label']===label);
const type=(label,value)=>{const field=input(label);field.value=value;field.oninput();};
const choose=label=>input(label).onchange();
(async()=>{
  await run('openTestFeedback(item)');
  assert.equal(input('처리할 AI').children.length,3);input('처리할 AI').value='claude';input('처리할 AI').onchange();
  assert.equal(byId('test-feedback-panel').hidden,false);assert.equal(input('확인한 항목').value,item.next);
  assert.equal(input('문제 상황과 재현 방법').parentElement.hidden,true);
  type('확인자','테스터');choose('이상 있음');assert.equal(input('문제 상황과 재현 방법').parentElement.hidden,false);
  await run("sendTestFeedback('submit')");assert.equal(starts,0);assert.match(byId('test-feedback-message').textContent,/문제 상황/);
  type('문제 상황과 재현 방법','저장 → 재개 후 원점으로 이동');choose('이상 없음');
  assert.equal(input('이상 없음').checked,true);assert.equal(input('이상 있음').checked,false);assert.equal(input('문제 상황과 재현 방법').parentElement.hidden,true);
  choose('이상 있음');await run('openTestFeedback(item)');assert.equal(input('확인자').value,'테스터');assert.equal(input('이상 있음').checked,true);assert.match(input('문제 상황과 재현 방법').value,/원점/);
  assert.equal(input('처리할 AI').value,'claude');
  loss=true;await run("sendTestFeedback('submit')");assert.equal(starts,1);assert.equal(savedRequest.result,'issues');assert.match(savedRequest.notes,/원점/);
  assert.equal(savedRequest.provider,'claude');
  assert.ok(storage.has('test:test-feedback:pending:'+item.path));assert.equal(byId('test-feedback-fields').disabled,true);assert.match(byId('test-feedback-message').textContent,/보존/);
  await run("sendTestFeedback('submit')");assert.equal(starts,1);assert.equal(storage.has('test:test-feedback:pending:'+item.path),false);
  assert.equal(byId('test-feedback-submit').disabled,true);assert.equal(input('이상 있음').checked,false);assert.equal(run('feedbackDraft(item.path).result'),'');
  assert.equal(input('문제 상황과 재현 방법').parentElement.hidden,true);
  latest={...latest,status:'retest',report:{summary:'복원 좌표 수정',changes:['좌표 복원 순서 수정'],checks:[{name:'회귀',status:'passed',evidence:'회귀 테스트 통과'}],humanChecks:['저장 후 복원 좌표 재확인'],blockers:[]}};revision++;
  await run('loadTestFeedback()');assert.equal(byId('test-feedback-submit').disabled,false);assert.equal(byId('test-feedback-fields').disabled,false);
  assert.match(byId('test-feedback-result').children.map(node=>node.textContent).join('\n'),/좌표 재확인/);assert.ok(recordsRendered>0);
  await run("sendTestFeedback('submit')");assert.equal(starts,1);assert.match(byId('test-feedback-message').textContent,/하나를 선택/);
  choose('이상 없음');refuse=true;await run("sendTestFeedback('submit')");assert.equal(storage.has('test:test-feedback:pending:'+item.path),false);assert.equal(input('이상 없음').checked,true);assert.equal(byId('test-feedback-fields').disabled,false);
  refuse=false;await run("sendTestFeedback('submit')");assert.equal(starts,2);assert.equal(savedRequest.notes,'','hidden issue notes must not be submitted as a passed result');
  latest={...latest,status:'failed',error:'로그인 확인 필요'};revision++;await run('loadTestFeedback()');
  input('처리할 AI').value='gemini';input('처리할 AI').onchange();
  const retry=byId('test-feedback-result').children.find(node=>node.textContent==='저장된 결과로 AI 다시 시도');assert.ok(retry);await retry.onclick();assert.equal(savedRequest.action,'retry');
  assert.equal(savedRequest.provider,'gemini');
  latest={...latest,status:'failed'};revision++;providers=[providers[0]];await run('loadTestFeedback()');
  assert.equal(input('처리할 AI').value,'gemini','unavailable selection must not silently fall back to another AI');
  const before=starts;await run("sendTestFeedback('retry')");assert.equal(starts,before);assert.match(byId('test-feedback-message').textContent,/연결되어 있지/);
  providers=undefined;await run('refreshFeedbackContext()');assert.equal(run('feedbackProviders().length'),1,'old server capabilities allow only the original Codex route');
  console.log('PASS feedback UI choices, required issue details, draft preservation, unknown-response replay, explicit failure, running lock, result refresh and retry');
})().catch(error=>{console.error(error);process.exitCode=1;});
