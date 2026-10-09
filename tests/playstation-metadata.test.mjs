import test from 'node:test';import assert from 'node:assert/strict';
import {enrich,titleKey} from '../tools/playstation-metadata.mjs';
test('Sony metadata preserves platform and sequel identity and never invents download counts',()=>{
 const cards=[{key:'a',title:'Call of Duty®: Black Ops III',platform:'ps4',downloads:-1},{key:'b',title:'Call of Duty Black Ops II',platform:'ps4',downloads:-1},{key:'c',title:'Call of Duty Black Ops III',platform:'ps5',downloads:-1}];
 const result=enrich(cards,{ratings:[{title:'Call of Duty Black Ops 3',rating:4.44,products:[],source:'https://store.playstation.com/test'}],charts:[{title:'Call of Duty Black Ops III',platform:'ps4',rank:12,period:'2026-08',region:'US/Canada',source:'https://blog.playstation.com/test'},{title:'Call of Duty Black Ops III',platform:'ps4',rank:2,period:'2026-07',region:'US/Canada'}]});
 assert.equal(result[0].rating,4.44);assert.equal(result[0].chartRank,12);assert.equal(result[0].downloads,-1);assert.equal(result[1].rating,undefined);assert.equal(result[2].chartRank,undefined);
});
test('unrated games remain in the enriched catalog, without placeholder scores',()=>{const cards=[{title:'Unknown',platform:'ps5'}];assert.deepEqual(enrich(cards,{ratings:[],charts:[]}),cards);assert.notEqual(titleKey('Black Ops II'),titleKey('Black Ops III'));});
