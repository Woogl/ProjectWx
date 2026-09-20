// Copyright Woogle. All Rights Reserved.
var WxWorkflowModel = (() => {
  const text = value => typeof value === 'string';
  const texts = value => Array.isArray(value) && value.length <= 200 && value.every(text);
  function validateResult(r) {
    if(r?.exclusions!==undefined && (!Array.isArray(r.exclusions)||r.exclusions.length>200||!r.exclusions.every(e=>e&&['id','reason','basis'].every(k=>text(e[k])&&e[k].trim()))||new Set(r.exclusions.map(e=>e.id)).size!==r.exclusions.length))throw new Error('범위 제외 자료 형식 오류');
    if(r?.questions?.some(q=>q.kind!==undefined&&!['planning','design'].includes(q.kind)))throw new Error('판단 단계 형식 오류');
    if (!r || !text(r.summary) || !text(r.draft) || !r.draft.trim() || !texts(r.facts) || !texts(r.evidence) || !texts(r.blockers) ||
        !Array.isArray(r.items) || r.items.length > 100 || !r.items.every(i => i && ['title','requirement','scope','acceptance'].every(k => text(i[k])) && texts(i.dependencies)) ||
        !Array.isArray(r.questions) || r.questions.length > 100 || !r.questions.every(q => q && text(q.id) && /^[A-Za-z0-9_-]{1,80}$/.test(q.id) &&
          ['title','requirement','scope','recommendation','impact','reopenReason'].every(k => text(q[k])) && q.title.trim() && q.scope.trim() && texts(q.options)) ||
        new Set(r.questions.map(q => q.id)).size !== r.questions.length) throw new Error('AI 응답 형식이 올바르지 않습니다. 기존 판단은 보존됩니다.');
    return r;
  }
  function active(q) { return !['excluded','transferred'].includes(q.status); }
  function mergeDecisions(previous, questions, exclusions = []) {
    const merged = JSON.parse(JSON.stringify(previous));
    for (const q of questions) {
      const old = merged.find(i => i.id === q.id);
      if (!old) { merged.push({ ...q, answer: '', confirmed: false, history: [] }); continue; }
      const changed = ['title','requirement','scope','recommendation','impact','options','kind'].some(k => JSON.stringify(old[k]) !== JSON.stringify(q[k]));
      if (old.confirmed && changed && !q.reopenReason.trim()) throw new Error(`${q.id}: 확정 판단 변경에는 재확인 이유가 필요합니다.`);
      if (q.reopenReason.trim() || changed) {
        old.history.push({ question: old.scope, answer: old.answer, confirmed: old.confirmed, reason: q.reopenReason || '질문 자료 변경' });
        Object.assign(old, q, { confirmed: false, status: 'active' });
      }
    }
    for (const removal of exclusions) {
      const old = merged.find(q => q.id === removal.id);
      if (!old || !text(removal.reason) || !removal.reason.trim() || !text(removal.basis) || !removal.basis.trim() || questions.some(q => q.id === removal.id)) throw new Error('제외할 판단과 인간의 범위 결정 근거를 확인하세요.');
      old.history.push({ question: old.scope, answer: old.answer, confirmed: old.confirmed, reason: removal.reason, basis: removal.basis });
      old.status = 'excluded'; old.exclusion = removal;
    }
    if (merged.length > 200) throw new Error('판단 항목이 200개를 초과했습니다. 작업 범위를 나누세요.');
    return merged;
  }
  function basis(source, notes, decisions) { return JSON.stringify({ source, notes, decisions }); }
  function taskName(value) {
    if(typeof value!=='string'||!value.trim()||value.trim().length>80||/[\\/:*?"<>|\x00-\x1f]/.test(value)||/[. ]$/.test(value))throw new Error('작업 제목은 1~80자이며 파일명에 사용할 수 없는 문자나 끝의 점·공백을 포함할 수 없습니다.');
    return value.trim();
  }
  function encodeFilename(value) { return encodeURIComponent(value).replace(/[!'()*]/g,c=>'%'+c.charCodeAt(0).toString(16).toUpperCase()); }
  function remapTask(value,oldName,newName) {
    const paths=text=>{
      for(const stage of ['planning','implementation','current','execution'])for(const extension of ['md','json']){
        const oldFile=`workflow_${oldName}_${stage}.${extension}`,newFile=`workflow_${newName}_${stage}.${extension}`;
        text=text.split(oldFile).join(newFile).split(encodeFilename(oldFile)).join(encodeFilename(newFile));
      }
      return text;
    };
    function walk(item,key='',parent={}) {
      if(Array.isArray(item))return item.map(entry=>walk(entry,key,parent));
      if(item&&typeof item==='object')return Object.fromEntries(Object.entries(item).map(([k,v])=>[k,walk(v,k,item)]));
      if(typeof item!=='string')return item;
      if(['taskId','newTaskId','changeTo'].includes(key)&&item===oldName)return newName;
      if(key==='title'&&parent.taskId===oldName)return newName;
      return ['path','dataPath','planning','implementation','design','basis','reviewBasis','prior'].includes(key)?paths(item):item;
    }
    return walk(value);
  }
  function ready(loop, source) {
    return !!(source.trim() && loop.result && !loop.result.blockers.length && loop.reviewBasis === basis(source, loop.notes, loop.decisions) && loop.decisions.every(q => !active(q) || (q.confirmed && q.answer.trim())));
  }
  return { validateResult, mergeDecisions, basis, ready, active, taskName, remapTask, encodeFilename };
})();
if (typeof module !== 'undefined') module.exports = WxWorkflowModel;
